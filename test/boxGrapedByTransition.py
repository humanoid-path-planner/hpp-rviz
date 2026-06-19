import numpy as np
from pinocchio import SE3, neutral
from pyhpp.constraints import ComparisonType, ComparisonTypes, LockedJoint
from pyhpp.core import SplineGradientBased_bezier3
from pyhpp.manipulation import (
    Device,
    Graph,
    Problem,
    Transition,
    TransitionPlanner,
    urdf,
)
from pyhpp.manipulation.constraint_graph_factory import ConstraintGraphFactory
from pyhpp_toppra import Toppra

from pyhpp_rviz import RVizVisualizer as Viewer

robot = Device("tuto")

urdf_filename = "package://example-robot-data/robots/panda_description/urdf/panda.urdf"
srdf_filename = "package://hpp_tutorial/srdf/panda.srdf"
urdf.loadModel(robot, 0, "hpp/panda", "anchor", urdf_filename, srdf_filename, SE3.Identity())

urdf_filename = "package://hpp_tutorial/urdf/ground.urdf"
srdf_filename = "package://hpp_tutorial/srdf/ground.srdf"
urdf.loadModel(robot, 0, "hpp/ground", "anchor", urdf_filename, srdf_filename, SE3.Identity())

urdf_filename = "package://hpp_tutorial/urdf/box.urdf"
srdf_filename = "package://hpp_tutorial/srdf/box.srdf"
urdf.loadModel(robot, 0, "hpp/box", "freeflyer", urdf_filename, srdf_filename, SE3.Identity())

robot.setJointBounds(
    "hpp/box/root_joint",
    [-1.5, 1.5, -1.5, 1.5, -0.2, 1.5,
     -float("Inf"), float("Inf"), -float("Inf"), float("Inf"),
     -float("Inf"), float("Inf"), -float("Inf"), float("Inf")],
)

q = neutral(robot.model())
q[-2:] = [0.035, 0.035]
q[9:12] = [0.4, -0.2, 0.025]

problem = Problem(robot)
graph = Graph("robot", robot, problem)
factory = ConstraintGraphFactory(graph)
graph.maxIterations(40)
graph.errorThreshold(1e-5)
factory.setGrippers(["hpp/panda/gripper"])
objects = ["hpp/box"]
handles_per_object = [["hpp/box/handle"]]
contacts_per_object = [["hpp/box/surface"]]
factory.setObjects(objects, handles_per_object, contacts_per_object)
factory.environmentContacts(["hpp/ground/surface"])
factory.generate()

cts = ComparisonTypes()
cts[:] = [ComparisonType.EqualToZero]
locked_fingers = []
for i in range(2):
    joint_name = f"hpp/panda/panda_finger_joint{i + 1}"
    lj = LockedJoint(robot, joint_name, np.array([0.035]), cts)
    locked_fingers.append(lj)
transition : Transition= graph.getTransition("hpp/panda/gripper > hpp/box/handle | f_23")
graph.addNumericalConstraintsToTransition(transition, locked_fingers)

graph.initialize()

q_init = np.array([0.0, 0.0, 0.0, -0.5, 0.0, 0.5, 0.0, 0.035, 0.035,
                   0.4, -0.2, 0.0251, 0.0, 0.0, 0.0, 1.0])

problem.initConfig(q_init)
problem.constraintGraph(graph)

shooter = problem.configurationShooter()

v = Viewer()
v.initViewer(robot=robot)
v(q_init)


def plan_transition(name, q_start, q_rhs=None):
    """
    Génère une configuration cible pour la transition `name`,
    puis planifie le chemin depuis q_start.

    q_start : configuration de départ du planificateur
    q_rhs   : configuration utilisée pour fixer le membre droit des contraintes
               (position de la boîte). Si None, utilise q_start.
    """
    if q_rhs is None:
        q_rhs = q_start

    transition = graph.getTransition(name)

    # Générer la configuration cible
    q_target = None
    for i in range(10000):
        q_shoot = shooter.shoot()
        res, q_target, err = graph.generateTargetConfig(transition, q_rhs, q_shoot)
        if not res:
            continue
        pv = transition.pathValidation()
        res_col, report = pv.validateConfiguration(q_target)
        if not res_col:
            continue
        print(f"  [{name}] config found at iteration {i}, err={err:.2e}")
        break
    else:
        raise RuntimeError(f"Failed to find target config for {name} after 10000 attempts")

    # Planifier le chemin
    q_goal_mat = np.zeros((1, robot.configSize()), order="F")
    q_goal_mat[0, :] = q_target
    tp = TransitionPlanner(problem)
    tp.setTransition(transition)
    tp.maxIterations(500)
    path = tp.planPath(q_start, q_goal_mat, True)

    # Lisser avec Bézier
    bezier = SplineGradientBased_bezier3(problem)
    path_smooth = bezier.optimize(path)
    print(f"  [{name}] Bézier: {path.length():.3f} → {path_smooth.length():.3f}")

    # Paramétrer en temps réel avec TOPPRA
    toppra = Toppra(problem)
    toppra.velocityScale = 0.5
    toppra.effortScale = -1
    toppra.N = 100
    toppra.accelerationLimits = np.array(15 * [0.5])
    path_timed = toppra.optimize(path_smooth)
    print(f"  [{name}] TOPPRA duration: {path_timed.length():.3f} s")

    return path_timed, q_target


# t1 : q_init → pregrasp
print("Planning t1: q_init → pregrasp")
t1, qpg = plan_transition("hpp/panda/gripper > hpp/box/handle | f_01", q_init)

# t2 : pregrasp → grasp  (RHS = boîte dans qpg)
print("Planning t2: pregrasp → grasp")
t2, qg = plan_transition("hpp/panda/gripper > hpp/box/handle | f_12", qpg, q_rhs=qpg)

# t3 : grasp → ... (RHS = boîte dans qg)

print("Planning t3: grasp → next")
t3, q3 = plan_transition("hpp/panda/gripper > hpp/box/handle | f_23", qg, q_rhs=qg)

# t4 : ... → ... (RHS = boîte dans q3)
print("Planning t4")
t4, q4 = plan_transition("hpp/panda/gripper > hpp/box/handle | f_34", q3, q_rhs=q3)

print("All paths planned.")

from pyhpp.core.path import Vector as PathVector

# Créer un PathVector vide
full_path = PathVector(robot.configSize(), robot.numberDof())

# Ajouter les chemins dans l'ordre
full_path.appendPath(t1)
full_path.appendPath(t2)
full_path.appendPath(t3)
full_path.appendPath(t4)

print(f"Durée totale: {full_path.length():.3f} s")

# Visualiser d'un coup
# v.loadPath(full_path)
# v.loadPath(t1)
# v.loadPath(t2)
# v.loadPath(t3)
# v.loadPath(t4)
