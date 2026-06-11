#include "StateTree/Tasks/Player/PlayerAcquireTargetTask.h"

/**
	- Get UOnsetPoolSubsystem from world                                                                                                                                         
	- Iterate GetActiveEnemies()                                                                                                                                                 
	- Score each: DistToHome <= MaxDistance && DistToSelf <= AcquireRange                                                                                                        
	- Pick closest valid target                                                                                                                                                  
	- Set target on TargetingComponent   
*/
