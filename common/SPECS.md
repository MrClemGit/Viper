Abstraction Layers
==================

The VIPER VM sits on top of two abstraction layers that isolate the VM core from the RTOS and hardware details.
The isolation is *opaque*, meaning that every details of the RTOS or of the hardware is completely hidden from the VM core. This approach surely adds a some performance reduction, however it enables the powerful mechanism of code UpLinking (see SPECS.md for the Uploader tool).


VOSAL: Viper Operative System Abstraction Layer
-----------------------------------------------

The VOSAL is completely defined in vosal.h and implemented in a RTOS-dependent vosal.c under common/vos/*rtos* directory. vosal.c includes _vos.h which is rtos and board dependent (it includes vboard.h in the config directory of the board) and prepares the environment for the RTOS to be built correctly.


### Init

The VOSAL provides a vosInit function that is responsible for the startup phase and the correct initialization of the underlying RTOS. vosInit also setup the memory management system initializing but not starting the garbage collector. The gc will be started by the VM itself and until that moment it will behave only as a heap. After vosInit gc_malloc and gc_free can be used.

### Threads

The VOSAL provides a thread abstraction where each thread (VThread) is considered to have a priority in the range [VOS_PRIO_IDLE,VOS_PRIO_HIGHEST]. Every VThread has a unique id and can also contain optional data that will be retrieved and used by the upper layers. A VThread can be in different states:

* INACTIVE: either the VThread has just been created or it has already terminated
* READY: VThread is active but not running and it is waiting to be scheduled. A VThread can become READY if it is started after creation (vosThResume) or if it is preempted by the scheduler or by a call to vosThYield
* RUNNING: VThread is executing
* SUSPENDED: VThread is waiting for an event and will not be scheduled again until that event happens. The event can be defined by a chThSleep or by a Semaphore/Mutex wait

### Semaphores and Mutexes

TBD

### Queues

TBD