from geometry_msgs.msg import TransformStamped
from rclpy.node import Node
from tf2_ros import TransformBroadcaster


class FreeFlyerPublisher(Node):
    """
    Publish freeflyer transforms on /tf via TransformBroadcaster.
    For each freeflyer joint, broadcast a TF from parent (ex:"world") to  Child frame (ex:"<namespace>/base_link").
    """

    def __init__(self):
        super().__init__("hpp_tf_broadcaster")
        self.broadcaster = TransformBroadcaster(self)
        self.last_transforms = {}

    def _publish(self):
        if not self.last_transforms:
            return
        transforms = []
        now = self.get_clock().now().to_msg()
        for child_frame, data in self.last_transforms.items():
            t = TransformStamped()
            t.header.stamp = now
            t.header.frame_id = data["parent_frame"]  # ex: "world"
            t.child_frame_id = child_frame  # ex: "box/base_link"
            t.transform.translation.x = data["xyz"][0]
            t.transform.translation.y = data["xyz"][1]
            t.transform.translation.z = data["xyz"][2]
            # Ordre Pinocchio freeflyer: [x, y, z, qx, qy, qz, qw]
            t.transform.rotation.x = data["quat"][0]
            t.transform.rotation.y = data["quat"][1]
            t.transform.rotation.z = data["quat"][2]
            t.transform.rotation.w = data["quat"][3]
            transforms.append(t)
        self.broadcaster.sendTransform(transforms)

    def publish(self, parent_frame, child_frame, xyz, quat_xyzw):
        """Publish a freeflyer transform from parent_frame to child_frame with given translation and rotation.
         The transform is updated at each call, only the last one is published.

        args:
        parent_frame : str, ex: "world"
        child_frame  : str, ex: "box/base_link"
        xyz          : tuple of 3 floats, translation (x, y, z)
        quat_xyzw    : tuple of 4 floats, rotation as quaternion (x, y, z, w)

        """
        self.last_transforms[child_frame] = {
            "parent_frame": parent_frame,
            "xyz": list(xyz),
            "quat": list(quat_xyzw),
        }
        self._publish()
