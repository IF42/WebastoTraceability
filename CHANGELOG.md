# 1.1.1 
Fixed bug in PLC2 Thread causing NULL function pointer call

# 1.1.0
This update makes project incompatible with older PLC software versions, because it is used modified PLC-PC communication interface!

* Improved and simplified PLC-PC interface
* Adding version into log init message for identifying of running version of traceability
* Adding cleaning command into command line interface for lock file deleting
* Improved application initialization

# 1.0.3
Improved performance of communication with PLC.

Improving of safety:
* Added protection for running multiple instances on one computer
* Added logging ability
* Fixed finilizing of sqlite statement in case of SQLITE_ERROR

# 1.0.2
Fixed inverted primer filling and primer expiration date-time in csv

# 1.0.1
Fixed bug in csv name and in csv data

# 1.0.0
Final release of software 
