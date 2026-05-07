import subprocess


def createRobotStatePublisherFromSubProcess(namespace, urdf_path):
    """Launch a robot_state_publisher as a subprocess, with the given namespace and urdf path.
    The robot_state_publisher will be launched with the following remappings and parameters:
    - __ns:=/<namespace>
    - frame_prefix:=<namespace>/"""
    ns = namespace.strip("/")

    subprocess.Popen(
        [
            "ros2",
            "run",
            "robot_state_publisher",
            "robot_state_publisher",
            f"{urdf_path}",
            "--ros-args",
            "--remap",
            f"__ns:=/{ns}",
            "-p",
            f"frame_prefix:={ns}/",
        ]
    )
