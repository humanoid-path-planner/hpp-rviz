import threading
import time
import warnings

import numpy as np
import pinocchio as pin
import pyhpp.core as core
import rclpy
from geometry_msgs.msg import PoseStamped
from hpp_msgs.msg import PathInfo, PinnochioJoint, PinnochioJointArray
from nav_msgs.msg import Path
from pinocchio.visualize import BaseVisualizer
from pyhpp import tools
from pyhpp.manipulation import Device, modelsInfo
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node

from .publisher.FreeFlyerPublisher import FreeFlyerPublisher
from .publisher.JointState import JointStatePublisher
from .publisher.Navigation import NavigationPublisher
from .publisher.RobotState import createRobotStatePublisherFromSubProcess
from .publisher.StaticTfPublisher import StaticTFPublisher


class RVizVisualizer(BaseVisualizer):
    """Pinocchio RViz2 visualizer (ROS 2)"""

    def __init__(self):
        self.robot: Device = None
        self.model: pin.Model = pin.Model()
        self.data: pin.Data = pin.Data()

        self.fixed_frame = "world"
        self.joint_map = {}  # name → (idx_q, type, child_frame_or_None)

        self.joint_state_publisher: JointStatePublisher = None
        self.static_tf_publisher: StaticTFPublisher = None
        self.navigation_publisher: NavigationPublisher = None
        self.freeFlyer_publisher: FreeFlyerPublisher = None

        self._executor = None
        self._spin_thread = None

        self.current_path = None
        self.last_vector_configuration = None

        self.path_info_pub = None
        self.pinnochioJoint_pub = None

    # ====================== Init ======================

    def initViewer(self, robot: Device = None):
        self.model = robot.model()
        self.data = self.model.createData()
        self.robot = robot

        if not rclpy.ok():
            rclpy.init()

        self.joint_state_publisher = JointStatePublisher()
        self.static_tf_publisher = StaticTFPublisher()
        self.navigation_publisher = NavigationPublisher()
        self.freeFlyer_publisher = FreeFlyerPublisher()

        self._joints_node = Node("hpp_pinnochio_joint_publisher")
        self._path_node = Node("hpp_pinnochio_path_publisher")

        # Subscriptions
        self.joint_state_publisher.create_subscription(
            PathInfo, "/hpp/trajectory_time", self._on_trajectory_time, 10
        )
        self._joints_node.create_subscription(
            PinnochioJoint, "/hpp/pinnochio_joints", self._on_joint_receive, 10
        )
        self._path_node.create_subscription(
            PathInfo, "/hpp/target_frame", self._on_target_frame, 10
        )

        # Publishers
        self.pinnochioJoint_pub = self._joints_node.create_publisher(
            PinnochioJointArray, "/hpp/scene_objects", 10
        )
        self.path_info_pub = self._path_node.create_publisher(
            PathInfo, "/hpp/pathInfo", 10
        )

        self._build_map_for_publisher()

        self._executor = MultiThreadedExecutor()
        for node in [
            self.joint_state_publisher,
            self.static_tf_publisher,
            self.navigation_publisher,
            self.freeFlyer_publisher,
            self._joints_node,
            self._path_node,
        ]:
            self._executor.add_node(node)

        self._spin_thread = threading.Thread(target=self._executor.spin, daemon=True)
        self._spin_thread.start()

        if robot.modelsInfo():
            for m in robot.modelsInfo():
                m.urdfPath = m.urdfPath or ""
                m.prefix = m.prefix or ""
                with open(tools.xacro.retrieve_resource(m.urdfPath)) as f:
                    robot_desc = f.read()
                if not robot_desc:
                    raise ValueError(
                        f"URDF file at {m.urdfPath} is empty or could not be read."
                    )
                createRobotStatePublisherFromSubProcess(
                    m.prefix, tools.xacro.retrieve_resource(m.urdfPath)
                )
            self._publish_root_static_transforms(robot.modelsInfo())

    # ====================== Static TF ======================

    def _publish_root_static_transforms(self, models: list[modelsInfo]):
        for m in models:
            ns = m.prefix.strip("/") if m.prefix else ""

            if any(
                jtype == "FREE_FLYER" and name.startswith(ns + "/")
                for name, (_, jtype, _, _) in self.joint_map.items()
            ):
                continue

            root_frame = self._get_first_body_frame_for_namespace(ns)
            if not root_frame:
                continue

            pose: pin.SE3 = m.pose
            self.static_tf_publisher.publish(
                parent_frame=self.fixed_frame,
                child_frame=root_frame,
                xyz=pose.translation,
                quat_xyzw=pin.Quaternion(pose.rotation).coeffs(),
            )
            print(f"[StaticTF] '{self.fixed_frame}' → '{root_frame}' (ns='{ns}')")

    def _get_first_body_frame_for_namespace(self, namespace: str) -> str | None:
        prefix = f"{namespace}/" if namespace else ""
        for frame in self.model.frames:
            if frame.type != pin.FrameType.BODY:
                continue
            if frame.name == "universe":
                continue
            if prefix and not frame.name.startswith(prefix):
                continue
            if not prefix and "/" in frame.name:
                continue
            return frame.name
        return None

    # ====================== Mapping joints ======================

    def _get_root_link_for_joint(self, joint_id: int) -> str:
        candidates = [
            f
            for f in self.model.frames
            if f.parentJoint == joint_id and f.type == pin.FrameType.BODY
        ]
        if not candidates:
            raise ValueError(
                f"No BODY frame found for joint_id={joint_id} "
                f"({self.model.names[joint_id]})"
            )
        if len(candidates) > 1:
            warnings.warn(
                f"Multiple BODY frames for joint {joint_id}, using first: "
                f"{[f.name for f in candidates]}"
            )
        return candidates[0].name

    def _build_map_for_publisher(self):
        self.joint_map = {}
        for joint_id in range(1, self.model.njoints):
            joint: pin.JointModel = self.model.joints[joint_id]
            name = self.model.names[joint_id]
            idx_q = joint.idx_q
            parts = name.rsplit("/", 1)
            namespace = parts[0] if len(parts) > 1 else ""
            joint_name = (
                parts[1] if len(parts) > 1 else name
            )  # shortname without namespace
            if joint.nq == 1:
                self.joint_map[name] = (idx_q, "JOINT", namespace, joint_name)
            elif joint.nq == 7:
                root_link = self._get_root_link_for_joint(joint_id)
                if root_link.startswith(namespace + "/"):
                    root_link = root_link[len(namespace) + 1 :]
                child_frame = f"{namespace}/{root_link}" if namespace else root_link
                self.joint_map[name] = (idx_q, "FREE_FLYER", namespace, child_frame)
            else:
                raise ValueError(f"Unsupported joint nq={joint.nq} for '{name}'")

    # ====================== Display ======================

    def __call__(self, q):
        self.display(q)

    def display(self, q=None):
        if q is None:
            return
        pin.forwardKinematics(self.model, self.data, q)
        pin.updateFramePlacements(self.model, self.data)

        if self.joint_state_publisher is not None:
            self._publish_scene(q)
        self.last_vector_configuration = q

    # ====================== Publication ======================

    def _publish_scene(self, q):
        q_vec = np.asarray(q).reshape(-1)
        array_msg = PinnochioJointArray()
        joint_states = {}  # namespace → {joint_name: position}

        for joint_id in range(1, self.model.njoints):
            name = self.model.names[joint_id]
            idx_q, jtype, namespace, extra = self.joint_map[name]
            min_val = float(self.model.lowerPositionLimit[idx_q])
            max_val = float(self.model.upperPositionLimit[idx_q])

            msg = PinnochioJoint()
            msg.name = name
            msg.type = jtype
            msg.min = min_val
            msg.max = max_val

            if jtype == "JOINT":
                msg.values = [float(q_vec[idx_q])]
                joint_states.setdefault(namespace, {})[extra] = float(q_vec[idx_q])
            elif jtype == "FREE_FLYER":
                vals = q_vec[idx_q : idx_q + 7]
                msg.values = vals.tolist()
                self.freeFlyer_publisher.publish(
                    parent_frame=self.fixed_frame,
                    child_frame=extra,
                    xyz=vals[:3],
                    quat_xyzw=vals[3:],
                )

            array_msg.joints.append(msg)

        for namespace, joints in joint_states.items():
            self.joint_state_publisher.publish(
                namespace, list(joints.keys()), list(joints.values())
            )
        self.pinnochioJoint_pub.publish(array_msg)

    def _on_joint_receive(self, msg: PinnochioJoint):
        if self.last_vector_configuration is None:
            return

        entry = self.joint_map.get(msg.name)
        if entry is None:
            return

        idx_q, jtype, _, _ = entry
        q = self.last_vector_configuration.copy()

        if jtype == "FREE_FLYER":
            vals = [float(v) for v in msg.values[:7]]
            quat = np.array(vals[3:7])
            norm = np.linalg.norm(quat)
            if norm > 1e-9:
                quat /= norm
            vals[3:7] = quat.tolist()
            q[idx_q : idx_q + 7] = vals

        elif jtype == "JOINT":
            q[idx_q] = msg.values[0]

        self.display(q)

    # ====================== Path ======================

    def loadPath(self, path: core.bindings.Path):
        self.current_path = path
        msg = PathInfo()
        msg.path_length = float(path.length())
        msg.frame_names = [frame.name for frame in self.model.frames]
        self.path_info_pub.publish(msg)

    def _on_trajectory_time(self, msg: PathInfo):
        if self.current_path is None:
            return
        t = np.clip(msg.current_time, 0.0, self.current_path.length())
        self.display(self.current_path.eval(t)[0])

    def _on_target_frame(self, msg: PathInfo):
        if self.current_path is None:
            return
        self.displayPath(
            self.current_path, topic_name="hpp_path", target_frame=msg.target_frame
        )

    def displayPath(
        self,
        path: core.bindings.Path,
        dt: float = 0.05,
        topic_name: str = "hpp_path",
        origin: str = "world",
        target_frame: str = None,
    ):
        if target_frame is None or target_frame == "":
            return
        frame_names = [f.name for f in self.model.frames]
        if target_frame not in frame_names:
            print(
                f"⚠️ Frame '{target_frame}' not found in model.\n Available frames: {frame_names}"
            )
            return

        now = self.joint_state_publisher.get_clock().now().to_msg()
        msg = Path()
        msg.header.frame_id = origin
        msg.header.stamp = now

        t = 0.0
        while t <= path.length() + 1e-6:
            q_vec = np.asarray(path.eval(t)[0]).reshape(-1)
            pin.forwardKinematics(self.model, self.data, q_vec)
            pin.updateFramePlacements(self.model, self.data)

            oMf = self.data.oMf[self.model.getFrameId(target_frame)]
            quat = pin.Quaternion(oMf.rotation)

            pose = PoseStamped()
            pose.header.frame_id = origin
            pose.header.stamp = now
            pose.pose.position.x = oMf.translation[0]
            pose.pose.position.y = oMf.translation[1]
            pose.pose.position.z = oMf.translation[2]
            pose.pose.orientation.x = quat.x
            pose.pose.orientation.y = quat.y
            pose.pose.orientation.z = quat.z
            pose.pose.orientation.w = quat.w
            msg.poses.append(pose)
            t += dt

        self.navigation_publisher.publish(path_msg=msg, topic_name=topic_name)

    def playPath(self, path: core.bindings.Path, dt: float = 0.002):
        def _play():
            t = 0.0
            while t <= path.length():
                self.display(path.eval(t)[0])
                t += dt
                time.sleep(dt)

        threading.Thread(target=_play, daemon=True).start()

    def printActualRvizVectorConfiguration(self, with_names: bool = True):
        if self.last_vector_configuration is None:
            print("No configuration available.")
            return

        q_map = {}
        for joint_id in range(1, self.model.njoints):
            name = self.model.names[joint_id]
            joint: pin.JointModel = self.model.joints[joint_id]
            if joint.nq == 1:
                q_map[joint.idx_q] = name
            elif joint.nq == 7:
                labels = ["x", "y", "z", "qx", "qy", "qz", "qw"]
                for i, label in enumerate(labels):
                    q_map[joint.idx_q + i] = f"{label} {name} (free-flyer)"

        print("[")
        for idx, value in enumerate(self.last_vector_configuration):
            suffix = f"  # {q_map[idx]}" if with_names and idx in q_map else ""
            print(f"\t{value},{suffix}")
        print("]")

    # ====================== Méthodes abstraites ======================

    def captureImage(self, w=None, h=None):
        pass

    def disableCameraControl(self):
        pass

    def enableCameraControl(self):
        pass

    def drawFrameVelocities(self, *args, **kwargs):
        pass

    def setBackgroundColor(self, *args, **kwargs):
        pass

    def setCameraPose(self, pose):
        pass

    def setCameraPosition(self, position):
        pass

    def setCameraTarget(self, target):
        pass

    def setCameraZoom(self, zoom):
        pass

    def displayCollisions(self, visibility: bool):
        pass

    def displayVisuals(self, visibility: bool):
        pass
