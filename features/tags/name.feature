@tags @bicycle
Feature: Bicycle - profile tag parsing

Background:
Given the profile "bicycle"

Scenario: Bike - name tag
When processing way tags I should get  
| name  | highway   | > | name  |
| Linea | cycleway  |   | Linea |
| Linea |           |   |       |
|       | cycleway  |   |       |
|       |           |   |       |

Scenario: Bike - highway and oneway tags
When processing way tags I should get  
| highway  | oneway | > | forward_mode | backward_mode |
| cycleway |        |   | 2            | 2             |
| cycleway | yes    |   | 2            | 6             |
| footway  |        |   | 6            | 6             |
| footway  | yes    |   | 6            | 6             |

Scenario: Bike - surface tags           
When processing way tags I should get  
| highway  | surface   | smoothness | > | forward_speed | backward_speed |
| cycleway |           |            |   | 15            | 15             |
| cycleway | compacted |            |   | 10            | 10             |
| footway  |           | bad        |   | 6             | 6              |
| footway  | compacted | bad        |   | 6             | 6              |
