/// \page rviz_viewer RViz2 Integration - hpp-gepetto-viewer

/// \section rviz_introduction Introduction

Le package `hpp-gepetto-viewer` fournit une intégration complète avec **RViz2** pour visualiser et interagir avec les robots, les trajectoires et les configurations HPP.

Cette fonctionnalité est particulièrement utile pour :
- Visualiser en temps réel les trajectoires générées par HPP
- Contrôler interactivement les configurations du robot
- Placer et éditer des waypoints de manière intuitive
- Synchroniser l’état du robot entre HPP et RViz

/// \section rviz_architecture Architecture ROS 2

```mermaid
graph TD
    subgraph "HPP / Python"
        RVizVis[RVizVisualizer<br/>(pyhpp_rviz)]
        PathPlayer[PathPlayer]
    end

    subgraph "RViz2 Plugins"
        TrajDisplay[TrajectoryDisplay + Panel]
        WaypointDisplay[WaypointDisplay]
        WaypointTool[Waypoint Tool]
    end

    subgraph "ROS Topics"
        PathInfo[/hpp/pathInfo<br/>/hpp/trajectory_time]
        SceneObj[/hpp/scene_objects<br/>(HppVectorConfiguration)]
        JointCtrl[/hpp/pinocchio_joints]
        WaypointSrv[/hpp_waypoint_server/...]
        TF[/tf + /tf_static]
        RobotDesc[/&lt;ns&gt;/robot_description]
    end

    RVizVis -->|publish| PathInfo
    RVizVis -->|publish| SceneObj
    RVizVis -->|publish| RobotDesc
    RVizVis -->|subscribe| JointCtrl
    RVizVis -->|subscribe| PathInfo

    TrajDisplay -->|control panel| TrajectorySlider
    WaypointTool -->|create markers| WaypointSrv
    WaypointDisplay -->|visibility| WaypointSrv
```

/// \section rviz_messages Messages Personnalisés

Les messages sont définis dans `src/hpp_rviz/msg/` :

- **`PathInfo.msg`**  
  ```msg
  std_msgs/Header header
  float64 path_length
  float64 current_time
  string[] frame_names
  string target_frame
  ```

- **`HppVectorConfiguration.msg`**  
  ```msg
  float64[] hpp_vector
  PinocchioJoint[] joints
  ```

- **`PinocchioJoint.msg`**  
  ```msg
  std_msgs/Header header
  string name
  string type          # "JOINT" ou "FREE_FLYER"
  float64[] values
  float64 min
  float64 max
  ```

- **`HppWaypoint.msg`**  
  ```msg
  std_msgs/Header header
  bool enable
  string name
  ```

/// \section rviz_python API Python (`pyhpp_rviz`)

```python
from pyhpp_rviz import RVizVisualizer

viewer = RVizVisualizer()
viewer.initViewer(robot)           # robot est un pyhpp.manipulation.Device

viewer(q)                          # Afficher une configuration
viewer.loadPath(path)              # Charger une trajectoire
viewer.displayPath(path, target_frame="gripper")

# Waypoints
viewer.addWaypoint([x, y, z], [qx, qy, qz, qw])
viewer.addWaypointFromFrame("gripper")
```

/// \section rviz_rviz Utilisation dans RViz2

1. Lancer RViz2
2. Ajouter le display **"HPP Trajectory"** → ouvre le panneau de contrôle
3. Ajouter le display **"HPP Waypoint"**
4. Activer l’outil **"HPP Waypoint Tool"** (raccourci clavier `W`)
5. Cliquer dans la vue 3D pour créer des waypoints interactifs

**Panneau de contrôle** (`TrajectorySlider`) :
- Slider + bouton Play/Pause
- Contrôle de vitesse
- Sélection du frame cible
- Sliders joints + free-flyer (avec normalisation quaternion)

/// \section rviz_build Compilation

Assurez-vous d’avoir l’option activée :

```bash
cmake -DBUILD_HPP_RVIZ_PKGS=ON ..
```

Dépendances ROS 2 requises :
- `rviz_common`, `rviz_default_plugins`, `rviz_rendering`
- `rosidl_default_generators`
- `pluginlib`, `interactive_markers`

/// \section rviz_troubleshooting Dépannage

**Problème** | **Solution**
--- | ---
Pas de robot visible | Vérifier que `robot_description` est publié sur `/&lt;ns&gt;/robot_description`
Pas de panneau de contrôle | Ajouter le display "HPP Trajectory"
Waypoints non visibles | Vérifier que le display "HPP Waypoint" est activé
Joint sliders vides | Publier sur `/hpp/scene_objects`
TF non publié | Vérifier `StaticTFPublisher`

**Commandes utiles :**
```bash
ros2 topic echo /hpp/scene_objects
ros2 topic echo /hpp/pathInfo
ros2 topic list | grep hpp
```

/// \section rviz_future Perspectives

- Support multi-robot plus robuste
- Visualisation des graphes de contraintes (en cours)
- Amélioration de l’édition interactive des waypoints
```

The file has been created successfully at `doc/rviz-viewer.hh`. You can download it from the file browser or attachments.