# A demo of some cool traffic logic in 3d space

Works with Unreal 5.4

All heavy AI logic is processed off the main thread using Runnables, this allows for large numbers of entities on screen with minimal performance impact.

Check out /Async to see the workers which process the logic

The original boid logic is from this project for UE4 (https://github.com/EncodedWorlds/Boids/)

This logic has been multithreaded and optimised by me, as well as entirely new logic in the TrafficManagers, TrafficEntities etc
