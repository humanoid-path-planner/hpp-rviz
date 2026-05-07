from rclpy.node import Node
from sensor_msgs.msg import JointState


class JointStatePublisher(Node):
    """Publish joint states on /joint_states (or /<namespace>/joint_states)"""

    def __init__(self):
        super().__init__("pinnochio_joint_state_publisher")
        self._pub_map = {}

    def _get_publisher(self, topic):
        if topic not in self._pub_map:
            self._pub_map[topic] = self.create_publisher(JointState, topic, 10)
        return self._pub_map[topic]

    def _topic_for(self, namespace):
        ns = namespace.strip("/")
        return f"{ns}/joint_states" if ns else "joint_states"

    def publish(self, namespace, names, positions):
        """Publish joint states on /<namespace>/joint_states.

        args:
            namespace : str, ex: "box"
            names     : list of str, joint names
            positions : list of float, joint positions
        """
        topic = self._topic_for(namespace)
        pub = self._get_publisher(topic)

        msg = JointState()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.name = list(names)
        msg.position = list(positions)
        pub.publish(msg)
