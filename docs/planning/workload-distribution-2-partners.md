# Project 5 Workload Distribution Form: 2 Partners

It is recommended that at least some of the functionality is a shared responsibility between two or more members of the team, whereas some of the functionality is assigned solely to an individual. That said, how you break up the project is entirely up to you, so long as the workload distribution (measured in points as defined below) is approximately balanced.

You are encouraged to support each other with an overall goal of successfully completing the project. If one person struggles with their responsibility, the other member is encouraged to step in. However, an agreement should be struck between the partners wherein one partner gives up some of the assigned points for that function relative to the amount of help received.  

You will fill out this form:
1. Before the project starts. This is to document your intention. 
2. Before the demo if there was any deviation from the initial plan.

| Requirement/specification                                                              | Points | Partner 1 | Partner 2  |
|----------------------------------------------------------------------------------------|--------|-----------|------------|
| Introduction                                                                           | 1      |           |     1      |
| Circuit diagram                                                                        | 1      |     1     |            |
| System architecture diagram                                                            | 1      |     1     |            |
| High-level flowchart                                                                   | 1      |     1     |            |
| Professional demo                                                                      | 2      |           |      2     |
| Collect ambient temperature using the LM19                                             | 2      |     2     |            |
| Collect data every 0.5 s (proof required)                                              | 1      |     1     |            |
| Collect the plant’s temperature using the LM92                                         | 6      |           |      6     |
| Collect data every 0.5 s (proof required)                                              | 1      |     1     |            |
| Collect the time spent in the present mode of operation using an I2C RTC               | 2      |           |      2     |
| Collect data every 1 s (proof required)                                                | 1      |           |      1     |
| Produce a moving average of the temperatures                                           | 5      |     5     |            |
| Regardless of the selected mode, the Peltier must be turned off after 5 minutes        | 2      |     2     |            |
| The Peltier device can be turned off                                                   | 2      |     2     |            |
| The Peltier device can be configured to heat                                           | 2      |     2     |            |
| The Peltier device can be configured to cool                                           | 2      |     2     |            |
| The Peltier device can be configured to match the ambient temperature                  | 2      |     2     |            |
| LCD displays the mode of operation                                                     | 2      |     2     |            |
| Display  the mode string in the upper-left corner                                      | 1      |           |      1     |
| Display the ambient temperature                                                        | 2      |           |      2     |
| Refresh the temperature at least every 2 s                                             | 1      |           |      1     |
| Display the temperature in the top-right corner                                        | 1      |           |      1     |
| Display the temperature with a resolution of 0.1 C                                     | 1      |           |      1     |
| Display the plant temperature                                                          | 2      |           |      2     |
| Refresh the temperature at least every 2 s                                             | 1      |           |      1     |
| Display the temperature in the bottom-right corner                                     | 1      |           |      1     |
| Display the temperature with a resolution of 0.1 C                                     | 1      |           |      1     |
| Display the window size                                                                | 2      |           |      2     |
| Display the time spent in the current mode of operation                                | 2      |           |      2     |
| Refresh the time at least every 1 s                                                    | 1      |           |      1     |
| Display the seconds in the bottom row of the LCD                                       | 1      |           |      1     |
| Display up to a 3-digit number                                                         | 1      |           |      1     |
| When the system is actively heating, the lights should fill to the right               | 2      |     2     |            |
| When the system is actively cooling, the lights should fill to the left                | 2      |     2     |            |
| When not actively heating or cooling, the lights should be off                         | 2      |     2     |            |
| Total                                                                                  | 60     |     30    |      30    |


## 🚀 Extra credit points
The extra credit points are allotted to both partners. You don't need to formally split these tasks. The points are just here for your information.

| Extra credit requirement/specification                                                                   | Extra Credit Points |
|----------------------------------------------------------------------------------------------------------|---------------------|
| The user can select the window size using the keypad                                                     | 1                   |
| The Peltier device can be configured to match a temperature entered using the keypad                     | 3                   |
| Display a custom character for the ambient temperature                                                   | 1                   |
| Display a custom character for the plant temperature                                                     | 1                   |
