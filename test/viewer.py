import numpy as np
from pinocchio import SE3, neutral
from pyhpp.constraints import ComparisonType, ComparisonTypes, LockedJoint
from pyhpp.manipulation import (
	Device,
	Graph,
	ManipulationPlanner,
	Problem,
	urdf,
)
from pyhpp.manipulation.constraint_graph_factory import ConstraintGraphFactory

from pyhpp_rviz import RVizVisualizer as Viewer

robot = Device("tuto")



urdf_filename = "package://example-robot-data/robots/panda_description/urdf/panda.urdf"
srdf_filename = "package://hpp_tutorial/srdf/panda.srdf"
urdf.loadModel(
	robot, 0, "panda", "anchor", urdf_filename, srdf_filename, SE3.Identity()
)



urdf_filename = "package://hpp_tutorial/urdf/ground.urdf"
srdf_filename = "package://hpp_tutorial/srdf/ground.srdf"
urdf.loadModel(
	robot, 0, "ground", "anchor", urdf_filename, srdf_filename, SE3.Identity()
)

urdf_filename = "package://hpp_tutorial/urdf/box.urdf"
srdf_filename = "package://hpp_tutorial/srdf/box.srdf"
urdf.loadModel(
	robot, 0, "box", "freeflyer", urdf_filename, srdf_filename, SE3.Identity()
)


robot.setJointBounds(
	"box/root_joint",
	[
		-1.5,
		1.5,
		-1.5,
		1.5,
		-0.2,
		1.5,
		-float("Inf"),
		float("Inf"),
		-float("Inf"),
		float("Inf"),
		-float("Inf"),
		float("Inf"),
		-float("Inf"),
		float("Inf"),
	],
)



q = neutral(robot.model())
q[-2:] = [0.035, 0.035]
q[9:12] = [0.4, -0.2, 0.025]


problem = Problem(robot)
graph = Graph("robot", robot, problem)
factory = ConstraintGraphFactory(graph)

graph.maxIterations(40)
graph.errorThreshold(1e-5)

factory.setGrippers(["panda/gripper"])
objects = ["box"]
handles_per_object = [["box/handle"]]
contacts_per_object = [["box/surface"]]
factory.setObjects(objects, handles_per_object, contacts_per_object)
factory.environmentContacts(["ground/surface"])
factory.generate()

cts = ComparisonTypes()
cts[:] = [ComparisonType.EqualToZero]
locked_fingers = []
for i in range(2):
	joint_name = f"panda/panda_finger_joint{i + 1}"
	lj = LockedJoint(robot, joint_name, np.array([0.035]), cts)
	locked_fingers.append(lj)

graph.addNumericalConstraintsToGraph(locked_fingers)
graph.initialize()


q_init = np.array(
	[
		0.0,
		0.0,
		0.0,
		-0.5,
		0.0,
		0.5,
		0.0,
		0.035,
		0.035,
		0.4,
		-0.2,
		0.0251,
		0.0,
		0.0,
		0.0,
		1.0,
	]
)
q_goal = np.array([ 0.   ,  0.   ,  0.   , -0.5  ,  0.   ,  0.5  ,  0.   ,  0.035,
        0.035,  0.4  ,  0.2  ,  0.0251,  0.   ,  0.   ,  0.   ,  1.   ])

problem.initConfig(q_init)
problem.addGoalConfig(q_goal)
problem.constraintGraph(graph)




v = Viewer()
v.initViewer(robot=robot)
v(q_init)
manipulation_planner = ManipulationPlanner(problem)
manipulation_planner.maxIterations(500)
path = manipulation_planner.solve()
