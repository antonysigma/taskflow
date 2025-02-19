#pragma once

#include "observer.hpp"
#include "taskflow.hpp"

/** 
@file executor.hpp
@brief executor include file
*/

namespace tf {

// ----------------------------------------------------------------------------
// Executor Definition
// ----------------------------------------------------------------------------

/** @class Executor

@brief class to create an executor for running a taskflow graph

An executor manages a set of worker threads to run one or multiple taskflows
using an efficient work-stealing scheduling algorithm.

@code{.cpp}
// Declare an executor and a taskflow
tf::Executor executor;
tf::Taskflow taskflow;

// Add three tasks into the taskflow
tf::Task A = taskflow.emplace([] () { std::cout << "This is TaskA\n"; });
tf::Task B = taskflow.emplace([] () { std::cout << "This is TaskB\n"; });
tf::Task C = taskflow.emplace([] () { std::cout << "This is TaskC\n"; });

// Build precedence between tasks
A.precede(B, C); 

tf::Future<void> fu = executor.run(taskflow);
fu.wait();                // block until the execution completes

executor.run(taskflow, [](){ std::cout << "end of 1 run"; }).wait();
executor.run_n(taskflow, 4);
executor.wait_for_all();  // block until all associated executions finish
executor.run_n(taskflow, 4, [](){ std::cout << "end of 4 runs"; }).wait();
executor.run_until(taskflow, [cnt=0] () mutable { return ++cnt == 10; });
@endcode

All the @c run methods are @em thread-safe. You can submit multiple
taskflows at the same time to an executor from different threads.
*/
class Executor {

  friend class FlowBuilder;
  friend class Subflow;

  //struct PerThread {
  //  Worker* worker;
  //  inline PerThread() : worker {nullptr} { }
  //};

  public:

    /**
    @brief constructs the executor with @c N worker threads

    The constructor spawns @c N worker threads to run tasks in a
    work-stealing loop. The number of workers must be greater than zero
    or an exception will be thrown.
    By default, the number of worker threads is equal to the maximum
    hardware concurrency returned by std::thread::hardware_concurrency.
    */
    explicit Executor(size_t N = std::thread::hardware_concurrency());
    
    /**
    @brief destructs the executor 

    The destructor calls Executor::wait_for_all to wait for all submitted
    taskflows to complete and then notifies all worker threads to stop
    and join these threads.
    */
    ~Executor();

    /**
    @brief runs a taskflow once
    
    @param taskflow a tf::Taskflow object

    @return a tf::Future that holds the result of the execution

    This member function executes the given taskflow once and returns a tf::Future
    object that eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run(taskflow);
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.

    @attention
    The executor does not own the given taskflow. It is your responsibility to
    ensure the taskflow remains alive during its execution.
    */
    tf::Future<void> run(Taskflow& taskflow);
    
    /**
    @brief runs a moved taskflow once
    
    @param taskflow a moved tf::Taskflow object

    @return a tf::Future that holds the result of the execution
    
    This member function executes a moved taskflow once and returns a tf::Future
    object that eventually holds the result of the execution.
    The executor will take care of the lifetime of the moved taskflow.

    @code{.cpp}
    tf::Future<void> future = executor.run(std::move(taskflow));
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    */
    tf::Future<void> run(Taskflow&& taskflow);

    /**
    @brief runs a taskflow once and invoke a callback upon completion

    @param taskflow a tf::Taskflow object 
    @param callable a callable object to be invoked after this run

    @return a tf::Future that holds the result of the execution
    
    This member function executes the given taskflow once and invokes the given
    callable when the execution completes.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run(taskflow, [](){ std::cout << "done"; });
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    
    @attention
    The executor does not own the given taskflow. It is your responsibility to
    ensure the taskflow remains alive during its execution.
    */
    template<typename C>
    tf::Future<void> run(Taskflow& taskflow, C&& callable);
    
    /**
    @brief runs a moved taskflow once and invoke a callback upon completion

    @param taskflow a moved tf::Taskflow object 
    @param callable a callable object to be invoked after this run

    @return a tf::Future that holds the result of the execution

    This member function executes a moved taskflow once and invokes the given
    callable when the execution completes.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.
    The executor will take care of the lifetime of the moved taskflow.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      std::move(taskflow), [](){ std::cout << "done"; }
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    */
    template<typename C>
    tf::Future<void> run(Taskflow&& taskflow, C&& callable);

    /**
    @brief runs a taskflow for @c N times
    
    @param taskflow a tf::Taskflow object
    @param N number of runs

    @return a tf::Future that holds the result of the execution
    
    This member function executes the given taskflow @c N times and returns a tf::Future
    object that eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run_n(taskflow, 2);  // run taskflow 2 times
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.

    @attention
    The executor does not own the given taskflow. It is your responsibility to
    ensure the taskflow remains alive during its execution.
    */
    tf::Future<void> run_n(Taskflow& taskflow, size_t N);
    
    /**
    @brief runs a moved taskflow for @c N times
    
    @param taskflow a moved tf::Taskflow object
    @param N number of runs

    @return a tf::Future that holds the result of the execution

    This member function executes a moved taskflow @c N times and returns a tf::Future
    object that eventually holds the result of the execution.
    The executor will take care of the lifetime of the moved taskflow.

    @code{.cpp}
    tf::Future<void> future = executor.run_n(
      std::move(taskflow), 2    // run the moved taskflow 2 times
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    */
    tf::Future<void> run_n(Taskflow&& taskflow, size_t N);

    /**
    @brief runs a taskflow for @c N times and then invokes a callback

    @param taskflow a tf::Taskflow 
    @param N number of runs
    @param callable a callable object to be invoked after this run

    @return a tf::Future that holds the result of the execution
    
    This member function executes the given taskflow @c N times and invokes the given
    callable when the execution completes.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      taskflow, 2, [](){ std::cout << "done"; }  // runs taskflow 2 times and invoke
                                                 // the lambda to print "done"
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    
    @attention
    The executor does not own the given taskflow. It is your responsibility to
    ensure the taskflow remains alive during its execution.
    */
    template<typename C>
    tf::Future<void> run_n(Taskflow& taskflow, size_t N, C&& callable);
    
    /**
    @brief runs a moved taskflow for @c N times and then invokes a callback

    @param taskflow a moved tf::Taskflow 
    @param N number of runs
    @param callable a callable object to be invoked after this run

    @return a tf::Future that holds the result of the execution
    
    This member function executes a moved taskflow @c N times and invokes the given
    callable when the execution completes.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      // run the moved taskflow 2 times and invoke the lambda to print "done"
      std::move(taskflow), 2, [](){ std::cout << "done"; }  
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    */
    template<typename C>
    tf::Future<void> run_n(Taskflow&& taskflow, size_t N, C&& callable);

    /**
    @brief runs a taskflow multiple times until the predicate becomes true

    @param taskflow a tf::Taskflow 
    @param pred a boolean predicate to return @c true for stop

    @return a tf::Future that holds the result of the execution
    
    This member function executes the given taskflow multiple times until
    the predicate returns @c true.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      taskflow, [](){ return rand()%10 == 0 }  
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    
    @attention
    The executor does not own the given taskflow. It is your responsibility to
    ensure the taskflow remains alive during its execution.
    */
    template<typename P>
    tf::Future<void> run_until(Taskflow& taskflow, P&& pred);
    
    /**
    @brief runs a moved taskflow and keeps running it 
           until the predicate becomes true

    @param taskflow a moved tf::Taskflow object
    @param pred a boolean predicate to return @c true for stop

    @return a tf::Future that holds the result of the execution
    
    This member function executes a moved taskflow multiple times until
    the predicate returns @c true.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.
    The executor will take care of the lifetime of the moved taskflow.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      std::move(taskflow), [](){ return rand()%10 == 0 }  
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    */
    template<typename P>
    tf::Future<void> run_until(Taskflow&& taskflow, P&& pred);

    /**
    @brief runs a taskflow multiple times until the predicate becomes true and 
           then invokes the callback

    @param taskflow a tf::Taskflow 
    @param pred a boolean predicate to return @c true for stop
    @param callable a callable object to be invoked after this run completes

    @return a tf::Future that holds the result of the execution
    
    This member function executes the given taskflow multiple times until
    the predicate returns @c true and then invokes the given callable when
    the execution completes.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      taskflow, [](){ return rand()%10 == 0 }, [](){ std::cout << "done"; } 
    );
    // do something else
    future.wait();
    @endcode
    
    This member function is thread-safe.
    
    @attention
    The executor does not own the given taskflow. It is your responsibility to
    ensure the taskflow remains alive during its execution.
    */
    template<typename P, typename C>
    tf::Future<void> run_until(Taskflow& taskflow, P&& pred, C&& callable);
    
    /**
    @brief runs a moved taskflow and keeps running 
           it until the predicate becomes true and then invokes the callback

    @param taskflow a moved tf::Taskflow
    @param pred a boolean predicate to return @c true for stop
    @param callable a callable object to be invoked after this run completes

    @return a tf::Future that holds the result of the execution

    This member function executes a moved taskflow multiple times until
    the predicate returns @c true and then invokes the given callable when
    the execution completes.
    This member function returns a tf::Future object that 
    eventually holds the result of the execution.
    The executor will take care of the lifetime of the moved taskflow.

    @code{.cpp}
    tf::Future<void> future = executor.run(
      std::move(taskflow), 
      [](){ return rand()%10 == 0 }, [](){ std::cout << "done"; } 
    );
    // do something else 
    future.wait();
    @endcode
    
    This member function is thread-safe.
    */
    template<typename P, typename C>
    tf::Future<void> run_until(Taskflow&& taskflow, P&& pred, C&& callable);

    /**
    @brief schedules a parented task immediatelly
    
    This member function immediately schedules the give task from a running
    taskflow graph. 
    The task must not be running.
    */
    void schedule(Task task);
    
    /**
    @brief wait for all tasks to complete

    This member function waits until all submitted tasks 
    (e.g., taskflows, asynchronous tasks) to finish.

    @code{.cpp}
    executor.run(taskflow1);
    executor.run_n(taskflow2, 10);
    executor.run_n(taskflow3, 100);
    executor.wait_for_all();  // wait until the above submitted taskflows finish
    @endcode
    */
    void wait_for_all();

    /**
    @brief queries the number of worker threads

    @code{.cpp}
    tf::Executor executor(4);
    std::cout << executor.num_workers();    // 4
    @endcode
    */
    size_t num_workers() const;
    
    /**
    @brief queries the number of running topologies at the time of this call

    When a taskflow is submitted to an executor, a topology is created to store
    runtime metadata of the running taskflow.
    When the execution of the submitted taskflow finishes,
    its corresponding topology will be removed from the executor.

    @code{.cpp}
    executor.run(taskflow);
    std::cout << executor.num_topologies();  // 0 or 1 (taskflow still running)
    @endcode
    */
    size_t num_topologies() const;

    /**
    @brief queries the number of running taskflows with moved ownership

    @code{.cpp}
    executor.run(std::move(taskflow));
    std::cout << executor.num_taskflows();  // 0 or 1 (taskflow still running)
    @endcode
    */
    size_t num_taskflows() const;

    /**
    @brief queries the id of the caller thread in this executor

    Each worker has an unique id in the range of @c 0 to @c N-1 associated with
    its parent executor.
    If the caller thread does not belong to the executor, @c -1 is returned.

    @code{.cpp}
    tf::Executor executor(4);   // 4 workers in the executor
    executor.this_worker_id();  // -1 (main thread is not a worker)

    taskflow.emplace([&](){
      std::cout << executor.this_worker_id();  // 0, 1, 2, or 3
    });
    executor.run(taskflow);
    @endcode
    */
    int this_worker_id() const;

    /** 
    @brief runs a given function asynchronously

    @tparam F callable type
    @tparam ArgsT parameter types

    @param f callable object to call
    @param args parameters to pass to the callable
    
    @return a tf::Future that will holds the result of the execution

    The method creates an asynchronous task to launch the given
    function on the given arguments.
    Unlike std::async, the return here is a @em tf::Future that holds
    an optional object to the result.
    If the asynchronous task is cancelled before it runs, the return is 
    a @c std::nullopt, or the value returned by the callable.

    @code{.cpp}
    tf::Fugure<std::optional<int>> future = executor.async([](){
      std::cout << "create an asynchronous task and returns 1\n";
      return 1;
    });
    @endcode

    This member function is thread-safe.
    */
    template <typename F, typename... ArgsT>
    auto async(F&& f, ArgsT&&... args);
    
    /** 
    @brief runs a given function asynchronously and gives a name to this task
    
    @tparam F callable type
    @tparam ArgsT parameter types

    @param name name of the asynchronous task
    @param f callable object to call
    @param args parameters to pass to the callable
    
    @return a tf::Future that will holds the result of the execution
    
    The method creates a named asynchronous task to launch the given
    function on the given arguments.
    Naming an asynchronous task is primarily used for profiling and visualizing
    the task execution timeline.
    Unlike std::async, the return here is a tf::Future that holds
    an optional object to the result.
    If the asynchronous task is cancelled before it runs, the return is 
    a @c std::nullopt, or the value returned by the callable.
    
    @code{.cpp}
    tf::Fugure<std::optional<int>> future = executor.named_async("name", [](){
      std::cout << "create an asynchronous task with a name and returns 1\n";
      return 1;
    });
    @endcode

    This member function is thread-safe.
    */
    template <typename F, typename... ArgsT>
    auto named_async(const std::string& name, F&& f, ArgsT&&... args);
    
    /**
    @brief similar to tf::Executor::async but does not return a future object

    This member function is more efficient than tf::Executor::async
    and is encouraged to use when there is no data returned.
    
    @code{.cpp}
    executor.silent_async([](){
      std::cout << "create an asynchronous task with no return\n";
    });
    @endcode

    This member function is thread-safe.
    */
    template <typename F, typename... ArgsT>
    void silent_async(F&& f, ArgsT&&... args);

    /**
    @brief similar to tf::Executor::named_async but does not return a future object

    This member function is more efficient than tf::Executor::named_async
    and is encouraged to use when there is no data returned.
    
    @code{.cpp}
    executor.named_silent_async("name", [](){
      std::cout << "create an asynchronous task with a name and no return\n";
    });
    @endcode

    This member function is thread-safe.
    */
    template <typename F, typename... ArgsT>
    void named_silent_async(const std::string& name, F&& f, ArgsT&&... args);
    
    /**
    @brief constructs an observer to inspect the activities of worker threads
    
    @tparam Observer observer type derived from tf::ObserverInterface
    @tparam ArgsT argument parameter pack

    @param args arguments to forward to the constructor of the observer
    
    @return a shared pointer to the created observer
    
    Each executor manages a list of observers with shared ownership with callers.
    For each of these observers, the two member functions,
    tf::ObserverInterface::on_entry and tf::ObserverInterface::on_exit
    will be called before and after the execution of a task.

    This member function is not thread-safe.
    */
    template <typename Observer, typename... ArgsT>
    std::shared_ptr<Observer> make_observer(ArgsT&&... args);
    
    /**
    @brief removes an observer from the executor

    This member function is not thread-safe.
    */
    template <typename Observer>
    void remove_observer(std::shared_ptr<Observer> observer);

    /**
    @brief queries the number of observers
    */
    size_t num_observers() const noexcept;
    
  private:

    std::condition_variable _topology_cv;
    std::mutex _taskflow_mutex;
    std::mutex _topology_mutex;
    std::mutex _wsq_mutex;

    size_t _num_topologies {0};
    
    std::vector<Worker> _workers;
    std::vector<std::thread> _threads;
    std::list<Taskflow> _taskflows;

    Notifier _notifier;

    TaskQueue<Node*> _wsq;

    std::atomic<size_t> _num_actives {0};
    std::atomic<size_t> _num_thieves {0};
    std::atomic<bool>   _done {0};
    
    std::unordered_set<std::shared_ptr<ObserverInterface>> _observers;

    bool _wait_for_task(Worker&, Node*&);
    
    void _observer_prologue(Worker&, Node*);
    void _observer_epilogue(Worker&, Node*);
    void _spawn(size_t);
    void _worker_loop(Worker&);
    void _exploit_task(Worker&, Node*&);
    void _explore_task(Worker&, Node*&);
    void _schedule(Worker*, Node*);
    void _schedule(Worker*, const SmallVector<Node*>&);
    void _set_up_topology(Worker*, Topology*);
    void _tear_down_topology(Worker&, Topology*); 
    void _tear_down_async(Node*);
    void _tear_down_invoke(Worker&, Node*, bool);
    void _increment_topology();
    void _decrement_topology();
    void _decrement_topology_and_notify();
    void _invoke(Worker&, Node*);
    void _invoke_static_task(Worker&, Node*);
    void _invoke_dynamic_task(Worker&, Node*);
    void _invoke_dynamic_task_internal(Worker&, Node*, Graph&, bool);
    void _invoke_dynamic_task_external(Node*, Graph&, bool);
    void _invoke_condition_task(Worker&, Node*, SmallVector<int>&);
    void _invoke_multi_condition_task(Worker&, Node*, SmallVector<int>&);
    void _invoke_module_task(Worker&, Node*);
    void _invoke_async_task(Worker&, Node*);
    void _invoke_silent_async_task(Worker&, Node*);
    void _invoke_cudaflow_task(Worker&, Node*);
    void _invoke_syclflow_task(Worker&, Node*);
    void _invoke_runtime_task(Worker&, Node*);

    template <typename C, 
      std::enable_if_t<is_cudaflow_task_v<C>, void>* = nullptr
    >
    void _invoke_cudaflow_task_entry(Node*, C&&);
    
    template <typename C, typename Q, 
      std::enable_if_t<is_syclflow_task_v<C>, void>* = nullptr
    >
    void _invoke_syclflow_task_entry(Node*, C&&, Q&);
};

// Constructor
inline Executor::Executor(size_t N) : 
  _workers    {N},
  _notifier   {N} {
  
  if(N == 0) {
    TF_THROW("no cpu workers to execute taskflows");
  }
  
  _spawn(N);

  // instantite the default observer if requested
  if(has_env(TF_ENABLE_PROFILER)) {
    TFProfManager::get()._manage(make_observer<TFProfObserver>());
  }
}

// Destructor
inline Executor::~Executor() {
  
  // wait for all topologies to complete
  wait_for_all();
  
  // shut down the scheduler
  _done = true;

  _notifier.notify(true);
  
  for(auto& t : _threads){
    t.join();
  } 
}

// Function: num_workers
inline size_t Executor::num_workers() const {
  return _workers.size();
}

// Function: num_topologies
inline size_t Executor::num_topologies() const {
  return _num_topologies;
}

// Function: num_taskflows
inline size_t Executor::num_taskflows() const {
  return _taskflows.size();
}

// Function: named_async
template <typename F, typename... ArgsT>
auto Executor::named_async(const std::string& name, F&& f, ArgsT&&... args) {

  _increment_topology();

  using T = std::invoke_result_t<F, ArgsT...>;
  using R = std::conditional_t<std::is_same_v<T, void>, void, std::optional<T>>;

  std::promise<R> p;

  auto tpg = std::make_shared<AsyncTopology>();

  Future<R> fu(p.get_future(), tpg);

  auto node = node_pool.animate(
    std::in_place_type_t<Node::Async>{},
    [p=make_moc(std::move(p)), f=std::forward<F>(f), args...] 
    (bool cancel) mutable {
      if constexpr(std::is_same_v<R, void>) {
        if(!cancel) {
          f(args...);
        }
        p.object.set_value();
      }
      else {
        p.object.set_value(cancel ? std::nullopt : std::make_optional(f(args...)));
      }
    },
    std::move(tpg)
  );

  node->_name = name;
  
  _schedule(this_worker().worker, node);

  return fu;
}

// Function: async
template <typename F, typename... ArgsT>
auto Executor::async(F&& f, ArgsT&&... args) {
  return named_async("", std::forward<F>(f), std::forward<ArgsT>(args)...);
}

// Function: named_silent_async
template <typename F, typename... ArgsT>
void Executor::named_silent_async(
  const std::string& name, F&& f, ArgsT&&... args
) {

  _increment_topology();

  Node* node = node_pool.animate(
    std::in_place_type_t<Node::SilentAsync>{},
    [f=std::forward<F>(f), args...] () mutable { 
      f(args...); 
    }
  );

  node->_name = name;
  
  _schedule(this_worker().worker, node);
}

// Function: silent_async
template <typename F, typename... ArgsT>
void Executor::silent_async(F&& f, ArgsT&&... args) {
  named_silent_async("", std::forward<F>(f), std::forward<ArgsT>(args)...);
}

// Function: this_worker_id
inline int Executor::this_worker_id() const {
  Worker* worker = this_worker().worker;
  return worker ? static_cast<int>(worker->_id) : -1;
}

// Procedure: _spawn
inline void Executor::_spawn(size_t N) {

  for(size_t id=0; id<N; ++id) {

    _workers[id]._id = id;
    _workers[id]._vtm = id;
    _workers[id]._executor = this;
    _workers[id]._waiter = &_notifier._waiters[id];
    
    _threads.emplace_back([this] (Worker& w) -> void {

      this_worker().worker = &w;

      Node* t = nullptr;

      // must use 1 as condition instead of !done
      while(1) {
        
        // execute the tasks.
        _exploit_task(w, t);

        // wait for tasks
        if(_wait_for_task(w, t) == false) {
          break;
        }
      }
      
    }, std::ref(_workers[id]));     
  }
}

// Function: _explore_task
inline void Executor::_explore_task(Worker& w, Node*& t) {
  
  //assert(_workers[w].wsq.empty());
  //assert(!t);

  size_t num_steals = 0;
  size_t num_yields = 0;
  size_t max_steals = ((_workers.size() + 1) << 1);

  std::uniform_int_distribution<size_t> rdvtm(0, _workers.size()-1);

  do {
    t = (w._id == w._vtm) ? _wsq.steal() : _workers[w._vtm]._wsq.steal();

    if(t) {
      break;
    }
    
    if(num_steals++ > max_steals) {
      std::this_thread::yield();
      if(num_yields++ > 100) {
        break;
      }
    }
    
    w._vtm = rdvtm(w._rdgen);
  } while(!_done);

}

// Procedure: _exploit_task
inline void Executor::_exploit_task(Worker& w, Node*& t) {
  
  if(t) {

    if(_num_actives.fetch_add(1) == 0 && _num_thieves == 0) {
      _notifier.notify(false);
    }

    while(t) {
      _invoke(w, t);
      t = w._wsq.pop();
    }

    --_num_actives;
  }
}

// Function: _wait_for_task
inline bool Executor::_wait_for_task(Worker& worker, Node*& t) {

  wait_for_task:

  //assert(!t);

  ++_num_thieves;

  explore_task:

  _explore_task(worker, t);

  if(t) {
    if(_num_thieves.fetch_sub(1) == 1) {
      _notifier.notify(false);
    }
    return true;
  }

  _notifier.prepare_wait(worker._waiter);
  
  //if(auto vtm = _find_vtm(me); vtm != _workers.size()) {
  if(!_wsq.empty()) {

    _notifier.cancel_wait(worker._waiter);
    //t = (vtm == me) ? _wsq.steal() : _workers[vtm].wsq.steal();
    
    t = _wsq.steal();  // must steal here
    if(t) {
      if(_num_thieves.fetch_sub(1) == 1) {
        _notifier.notify(false);
      }
      return true;
    }
    else {
      worker._vtm = worker._id;
      goto explore_task;
    }
  }

  if(_done) {
    _notifier.cancel_wait(worker._waiter);
    _notifier.notify(true);
    --_num_thieves;
    return false;
  }

  if(_num_thieves.fetch_sub(1) == 1) {
    if(_num_actives) {
      _notifier.cancel_wait(worker._waiter);
      goto wait_for_task;
    }
    // check all queues again
    for(auto& w : _workers) {
      if(!w._wsq.empty()) {
        worker._vtm = w._id;
        _notifier.cancel_wait(worker._waiter);
        goto wait_for_task;
      }
    }
  }
    
  // Now I really need to relinguish my self to others
  _notifier.commit_wait(worker._waiter);

  return true;
}

// Function: make_observer    
template<typename Observer, typename... ArgsT>
std::shared_ptr<Observer> Executor::make_observer(ArgsT&&... args) {

  static_assert(
    std::is_base_of_v<ObserverInterface, Observer>,
    "Observer must be derived from ObserverInterface"
  );
  
  // use a local variable to mimic the constructor 
  auto ptr = std::make_shared<Observer>(std::forward<ArgsT>(args)...);
  
  ptr->set_up(_workers.size());

  _observers.emplace(std::static_pointer_cast<ObserverInterface>(ptr));

  return ptr;
}

// Procedure: remove_observer
template <typename Observer>
void Executor::remove_observer(std::shared_ptr<Observer> ptr) {
  
  static_assert(
    std::is_base_of_v<ObserverInterface, Observer>,
    "Observer must be derived from ObserverInterface"
  );

  _observers.erase(std::static_pointer_cast<ObserverInterface>(ptr));
}

// Function: num_observers
inline size_t Executor::num_observers() const noexcept {
  return _observers.size();
}
  
// Procedure: schedule
inline void Executor::schedule(Task task) { 
  auto node = task._node; 
  auto& j = node->_parent ? node->_parent->_join_counter : 
                            node->_topology->_join_counter;
  j.fetch_add(1);
  _schedule(this_worker().worker, node);
}

// Procedure: _schedule
// The main procedure to schedule a give task node.
// Each task node has two types of tasks - regular and subflow.
inline void Executor::_schedule(Worker* worker, Node* node) {
  
  //assert(_workers.size() != 0);
  
  node->_state.fetch_or(Node::READY, std::memory_order_release);

  // caller is a worker to this pool
  //auto worker = this_worker().worker;

  if(worker != nullptr && worker->_executor == this) {
    worker->_wsq.push(node);
    return;
  }

  // other threads push the node into the globally shared queue
  // since there will be no threads popping nodes from the queue,
  // we need to call uncached_push
  {
    std::lock_guard<std::mutex> lock(_wsq_mutex);
    _wsq.uncached_push(node);
  }

  _notifier.notify(false);
}

// Procedure: _schedule
// The main procedure to schedule a set of task nodes.
// Each task node has two types of tasks - regular and subflow.
inline void Executor::_schedule(Worker* worker, const SmallVector<Node*>& nodes) {

  //assert(_workers.size() != 0);
  
  // We need to cacth the node count to avoid accessing the nodes
  // vector while the parent topology is removed!
  const auto num_nodes = nodes.size();
  
  if(num_nodes == 0) {
    return;
  }
  
  // make the node ready
  for(size_t i=0; i<num_nodes; ++i) {
    nodes[i]->_state.fetch_or(Node::READY, std::memory_order_release);
  }

  // worker thread
  //auto worker = this_worker().worker;

  if(worker != nullptr && worker->_executor == this) {
    for(size_t i=0; i<num_nodes; ++i) {
      worker->_wsq.push(nodes[i]);
    }
    return;
  }
  
  // other threads push the node into the globally shared queue
  // since there will be no threads popping nodes from the queue,
  // we need to call uncached_push
  {
    std::lock_guard<std::mutex> lock(_wsq_mutex);
    for(size_t k=0; k<num_nodes; ++k) {
      _wsq.uncached_push(nodes[k]);
    }
  }
  
  _notifier.notify_n(num_nodes);
}

// Procedure: _invoke
inline void Executor::_invoke(Worker& worker, Node* node) {
  
  // synchronize all outstanding memory operations caused by reordering
  while(!(node->_state.load(std::memory_order_acquire) & Node::READY));
  
  // no need to do other things if the topology is cancelled
  //if(node->_topology && node->_topology->_is_cancelled) {
  if(node->_is_cancelled()) {
    _tear_down_invoke(worker, node, true);
    return;
  }

  // if acquiring semaphore(s) exists, acquire them first
  if(node->_semaphores && !node->_semaphores->to_acquire.empty()) {
    SmallVector<Node*> nodes;
    if(!node->_acquire_all(nodes)) {
      _schedule(&worker, nodes);
      return;
    }
    //node->_set_state(Node::ACQUIRED);
    node->_state.fetch_or(Node::ACQUIRED, std::memory_order_release);
  }

  // Here we need to fetch the num_successors first to avoid the invalid memory
  // access caused by topology clear.
  const auto num_successors = node->num_successors();
  
  // condition task
  //int cond = -1;
  SmallVector<int> conds = { -1 };
  
  // switch is faster than nested if-else due to jump table
  switch(node->_handle.index()) {
    // static task
    case Node::STATIC:{
      _invoke_static_task(worker, node);
    } 
    break;
    
    // dynamic task
    case Node::DYNAMIC: {
      _invoke_dynamic_task(worker, node);
    }
    break;
    
    // condition task
    case Node::CONDITION: {
      _invoke_condition_task(worker, node, conds);
    }
    break;
    
    // multi-condition task
    case Node::MULTI_CONDITION: {
      _invoke_multi_condition_task(worker, node, conds);
    }
    break;

    // module task
    case Node::MODULE: {
      _invoke_module_task(worker, node);
    }
    break;

    // async task
    case Node::ASYNC: {
      _invoke_async_task(worker, node);
      _tear_down_invoke(worker, node, false);
      return ;
    }
    break;
    
    // silent async task
    case Node::SILENT_ASYNC: {
      _invoke_silent_async_task(worker, node);
      _tear_down_invoke(worker, node, false);
      return ;
    }
    break;

    // cudaflow task
    case Node::CUDAFLOW: {
      _invoke_cudaflow_task(worker, node);
    }
    break; 
    
    // syclflow task
    case Node::SYCLFLOW: {
      _invoke_syclflow_task(worker, node);
    }
    break; 

    // runtime task
    case Node::RUNTIME: {
      _invoke_runtime_task(worker, node);
    }
    break;

    // monostate (placeholder)
    default:
    break;
  }

  // if releasing semaphores exist, release them
  if(node->_semaphores && !node->_semaphores->to_release.empty()) {
    _schedule(&worker, node->_release_all());
  }

  // We MUST recover the dependency since the graph may have cycles.
  // This must be done before scheduling the successors, otherwise this might cause 
  // race condition on the _dependents
  //if(node->_has_state(Node::CONDITIONED)) {
  if((node->_state.load(std::memory_order_relaxed) & Node::CONDITIONED)) {
    node->_join_counter = node->num_strong_dependents();
  }
  else {
    node->_join_counter = node->num_dependents();
  }
  
  // acquire the parent flow counter
  auto& j = (node->_parent) ? node->_parent->_join_counter : 
                              node->_topology->_join_counter;
  
  // At this point, the node storage might be destructed (to be verified)
  // case 1: non-condition task
  switch(node->_handle.index()) {

    // condition and multi-condition tasks
    case Node::CONDITION: 
    case Node::MULTI_CONDITION: {
      for(auto cond : conds) {
        if(cond >= 0 && static_cast<size_t>(cond) < num_successors) {
          auto s = node->_successors[cond];
          // zeroing the join counter seems redundant but it keeps invariant
          s->_join_counter.store(0, std::memory_order_relaxed);
          j.fetch_add(1);
          _schedule(&worker, s);
        }
      }
    }
    break;
    
    // non-condition task
    default: {
      for(size_t i=0; i<num_successors; ++i) {
        if(--(node->_successors[i]->_join_counter) == 0) {
          j.fetch_add(1);
          _schedule(&worker, node->_successors[i]);
        }
      }
    }
    break;
  }

  // tear_down the invoke
  _tear_down_invoke(worker, node, false);
}

// Procedure: _tear_down_async
inline void Executor::_tear_down_async(Node* node) {
  if(node->_parent) {  
    node->_parent->_join_counter.fetch_sub(1);
  }
  else {
    _decrement_topology_and_notify();
  }
  node_pool.recycle(node);
}

// Procedure: _tear_down_invoke
inline void Executor::_tear_down_invoke(Worker& worker, Node* node, bool cancel) {

  switch(node->_handle.index()) {
    // async task needs to carry out the promise
    case Node::ASYNC:
      if(cancel) {
        std::get_if<Node::Async>(&(node->_handle))->work(true);
      }
      _tear_down_async(node);
    break;

    // silent async doesn't need to carry out the promise
    case Node::SILENT_ASYNC:
      _tear_down_async(node);
    break;

    // tear down topology if the node is the last leaf
    default: {
      if(node->_parent == nullptr) {
        if(node->_topology->_join_counter.fetch_sub(1) == 1) {
          _tear_down_topology(worker, node->_topology);
        }
      }
      else {  // joined subflow
        node->_parent->_join_counter.fetch_sub(1);
      }
    }
    break;
  }
}

// Procedure: _observer_prologue
inline void Executor::_observer_prologue(Worker& worker, Node* node) {
  for(auto& observer : _observers) {
    observer->on_entry(WorkerView(worker), TaskView(*node));
  }
}

// Procedure: _observer_epilogue
inline void Executor::_observer_epilogue(Worker& worker, Node* node) {
  for(auto& observer : _observers) {
    observer->on_exit(WorkerView(worker), TaskView(*node));
  }
}

// Procedure: _invoke_static_task
inline void Executor::_invoke_static_task(Worker& worker, Node* node) {
  _observer_prologue(worker, node);
  std::get_if<Node::Static>(&node->_handle)->work();
  _observer_epilogue(worker, node);
}

// Procedure: _invoke_dynamic_task
inline void Executor::_invoke_dynamic_task(Worker& w, Node* node) {

  _observer_prologue(w, node);

  auto handle = std::get_if<Node::Dynamic>(&node->_handle);

  handle->subgraph.clear();

  Subflow sf(*this, node, handle->subgraph); 

  handle->work(sf);

  if(sf._joinable) {
    _invoke_dynamic_task_internal(w, node, handle->subgraph, false);
  }
  
  _observer_epilogue(w, node);
}

// Procedure: _invoke_dynamic_task_external
inline void Executor::_invoke_dynamic_task_external(Node*p, Graph& g, bool detach) {

  auto worker = this_worker().worker;

  //assert(worker && worker->_executor == this);
  
  _invoke_dynamic_task_internal(*worker, p, g, detach);
}

// Procedure: _invoke_dynamic_task_internal
inline void Executor::_invoke_dynamic_task_internal(
  Worker& w, Node* p, Graph& g, bool detach
) {

  // graph is empty and has no async tasks
  if(g.empty() && p->_join_counter == 0) {
    return;
  }

  SmallVector<Node*> src; 

  for(auto n : g._nodes) {

    n->_topology = p->_topology;
    n->_set_up_join_counter();

    if(detach) {
      n->_parent = nullptr;
      n->_state.fetch_or(Node::DETACHED, std::memory_order_relaxed);
    }
    else {
      n->_parent = p;
    }
    
    if(n->num_dependents() == 0) {
      src.push_back(n);
    }
  }

  // detach here
  if(detach) {    
    
    {
      std::lock_guard<std::mutex> lock(p->_topology->_taskflow._mutex);
      p->_topology->_taskflow._graph.merge(std::move(g));
    }

    p->_topology->_join_counter.fetch_add(src.size());
    _schedule(&w, src);
  }
  // join here
  else {  
    p->_join_counter.fetch_add(src.size());
    _schedule(&w, src);
    Node* t = nullptr;
  
    std::uniform_int_distribution<size_t> rdvtm(0, _workers.size()-1);

    while(p->_join_counter != 0) {

      t = w._wsq.pop();

      exploit:

      if(t) {
        _invoke(w, t);
      }
      else {
        explore:
        t = (w._id == w._vtm) ? _wsq.steal() : _workers[w._vtm]._wsq.steal();
        if(t) {
          goto exploit;
        }
        else if(p->_join_counter != 0){
          std::this_thread::yield();
          w._vtm = rdvtm(w._rdgen);
          goto explore;
        }
        else {
          break;
        }
      }
    }
  }
}

// Procedure: _invoke_condition_task
inline void Executor::_invoke_condition_task(
  Worker& worker, Node* node, SmallVector<int>& conds
) {
  _observer_prologue(worker, node);
  conds[0] = std::get_if<Node::Condition>(&node->_handle)->work();
  _observer_epilogue(worker, node);
}

// Procedure: _invoke_multi_condition_task
inline void Executor::_invoke_multi_condition_task(
  Worker& worker, Node* node, SmallVector<int>& conds
) {
  _observer_prologue(worker, node);
  conds = std::get_if<Node::MultiCondition>(&node->_handle)->work();
  _observer_epilogue(worker, node);
}

// Procedure: _invoke_cudaflow_task
inline void Executor::_invoke_cudaflow_task(Worker& worker, Node* node) {
  _observer_prologue(worker, node);  
  std::get_if<Node::cudaFlow>(&node->_handle)->work(*this, node);
  _observer_epilogue(worker, node);
}

// Procedure: _invoke_syclflow_task
inline void Executor::_invoke_syclflow_task(Worker& worker, Node* node) {
  _observer_prologue(worker, node);  
  std::get_if<Node::syclFlow>(&node->_handle)->work(*this, node);
  _observer_epilogue(worker, node);
}

// Procedure: _invoke_module_task
inline void Executor::_invoke_module_task(Worker& w, Node* node) {
  _observer_prologue(w, node);
  //auto& module = std::get_if<Node::Module>(&node->_handle)->module;
  //_invoke_dynamic_task_internal(w, node, module._graph, false);
  //auto& graph = std::get_if<Node::Module>(&node->_handle)->module._graph;
  auto& graph = std::get_if<Node::Module>(&node->_handle)->graph;
  _invoke_dynamic_task_internal(w, node, graph, false);
  _observer_epilogue(w, node);  
}

// Procedure: _invoke_async_task
inline void Executor::_invoke_async_task(Worker& w, Node* node) {
  _observer_prologue(w, node);
  std::get_if<Node::Async>(&node->_handle)->work(false);
  _observer_epilogue(w, node);  
}

// Procedure: _invoke_silent_async_task
inline void Executor::_invoke_silent_async_task(Worker& w, Node* node) {
  _observer_prologue(w, node);
  std::get_if<Node::SilentAsync>(&node->_handle)->work();
  _observer_epilogue(w, node);  
}

// Procedure: _invoke_runtime_task
inline void Executor::_invoke_runtime_task(Worker& w, Node* node) {
  _observer_prologue(w, node);
  Runtime rt(*this);
  std::get_if<Node::Runtime>(&node->_handle)->work(rt);
  _observer_epilogue(w, node);  
}

// Function: run
inline tf::Future<void> Executor::run(Taskflow& f) {
  return run_n(f, 1, [](){});
}

// Function: run
inline tf::Future<void> Executor::run(Taskflow&& f) {
  return run_n(std::move(f), 1, [](){});
}

// Function: run
template <typename C>
tf::Future<void> Executor::run(Taskflow& f, C&& c) {
  return run_n(f, 1, std::forward<C>(c));
}

// Function: run
template <typename C>
tf::Future<void> Executor::run(Taskflow&& f, C&& c) {
  return run_n(std::move(f), 1, std::forward<C>(c));
}

// Function: run_n
inline tf::Future<void> Executor::run_n(Taskflow& f, size_t repeat) {
  return run_n(f, repeat, [](){});
}

// Function: run_n
inline tf::Future<void> Executor::run_n(Taskflow&& f, size_t repeat) {
  return run_n(std::move(f), repeat, [](){});
}

// Function: run_n
template <typename C>
tf::Future<void> Executor::run_n(Taskflow& f, size_t repeat, C&& c) {
  return run_until(
    f, [repeat]() mutable { return repeat-- == 0; }, std::forward<C>(c)
  );
}

// Function: run_n
template <typename C>
tf::Future<void> Executor::run_n(Taskflow&& f, size_t repeat, C&& c) {
  return run_until(
    std::move(f), [repeat]() mutable { return repeat-- == 0; }, std::forward<C>(c)
  );
}

// Function: run_until    
template<typename P>
tf::Future<void> Executor::run_until(Taskflow& f, P&& pred) {
  return run_until(f, std::forward<P>(pred), [](){});
}

// Function: run_until    
template<typename P>
tf::Future<void> Executor::run_until(Taskflow&& f, P&& pred) {
  return run_until(std::move(f), std::forward<P>(pred), [](){});
}

// Function: run_until
template <typename P, typename C>
tf::Future<void> Executor::run_until(Taskflow& f, P&& p, C&& c) {

  _increment_topology();
  
  // Need to check the empty under the lock since dynamic task may
  // define detached blocks that modify the taskflow at the same time
  bool empty;
  {
    std::lock_guard<std::mutex> lock(f._mutex);
    empty = f.empty();
  }
  
  // No need to create a real topology but returns an dummy future
  if(empty || p()) {
    c();
    std::promise<void> promise;
    promise.set_value();
    _decrement_topology_and_notify();
    return tf::Future<void>(promise.get_future(), std::monostate{});
  }
  
  // create a topology for this run
  auto t = std::make_shared<Topology>(f, std::forward<P>(p), std::forward<C>(c));
  
  // need to create future before the topology got torn down quickly
  tf::Future<void> future(t->_promise.get_future(), t);
  
  // modifying topology needs to be protected under the lock
  {
    std::lock_guard<std::mutex> lock(f._mutex);
    f._topologies.push(t);
    if(f._topologies.size() == 1) {
      _set_up_topology(this_worker().worker, t.get());
    }
  }

  return future;
}

// Function: run_until
template <typename P, typename C>
tf::Future<void> Executor::run_until(Taskflow&& f, P&& pred, C&& c) {

  std::list<Taskflow>::iterator itr;

  {
    std::scoped_lock<std::mutex> lock(_taskflow_mutex);
    itr = _taskflows.emplace(_taskflows.end(), std::move(f));
    itr->_satellite = itr;
  }

  return run_until(*itr, std::forward<P>(pred), std::forward<C>(c));
}

// Procedure: _increment_topology
inline void Executor::_increment_topology() {
  std::lock_guard<std::mutex> lock(_topology_mutex);
  ++_num_topologies;
}

// Procedure: _decrement_topology_and_notify
inline void Executor::_decrement_topology_and_notify() {
  std::lock_guard<std::mutex> lock(_topology_mutex);
  if(--_num_topologies == 0) {
    _topology_cv.notify_all();
  }
}

// Procedure: _decrement_topology
inline void Executor::_decrement_topology() {
  std::lock_guard<std::mutex> lock(_topology_mutex);
  --_num_topologies;
}

// Procedure: wait_for_all
inline void Executor::wait_for_all() {
  std::unique_lock<std::mutex> lock(_topology_mutex);
  _topology_cv.wait(lock, [&](){ return _num_topologies == 0; });
}

// Function: _set_up_topology
inline void Executor::_set_up_topology(Worker* worker, Topology* tpg) {

  // ---- under taskflow lock ----

  tpg->_sources.clear();
  tpg->_taskflow._graph.clear_detached();
  
  // scan each node in the graph and build up the links
  for(auto node : tpg->_taskflow._graph._nodes) {
    
    node->_topology = tpg;
    node->_state.store(0, std::memory_order_relaxed);

    if(node->num_dependents() == 0) {
      tpg->_sources.push_back(node);
    }

    node->_set_up_join_counter();
  }

  tpg->_join_counter = tpg->_sources.size();
  _schedule(worker, tpg->_sources);
}

// Function: _tear_down_topology
inline void Executor::_tear_down_topology(Worker& worker, Topology* tpg) {

  auto &f = tpg->_taskflow;

  //assert(&tpg == &(f._topologies.front()));

  // case 1: we still need to run the topology again
  if(!tpg->_is_cancelled && !tpg->_pred()) {
    //assert(tpg->_join_counter == 0);
    std::lock_guard<std::mutex> lock(f._mutex);
    tpg->_join_counter = tpg->_sources.size();
    _schedule(&worker, tpg->_sources); 
  }
  // case 2: the final run of this topology
  else {

    // TODO: if the topology is cancelled, need to release all semaphores
    
    if(tpg->_call != nullptr) {
      tpg->_call();
    }

    // If there is another run (interleave between lock)
    if(std::unique_lock<std::mutex> lock(f._mutex); f._topologies.size()>1) {
      //assert(tpg->_join_counter == 0);

      // Set the promise
      tpg->_promise.set_value();
      f._topologies.pop();
      tpg = f._topologies.front().get();
      
      // decrement the topology but since this is not the last we don't notify
      _decrement_topology();
      
      // set up topology needs to be under the lock or it can
      // introduce memory order error with pop
      _set_up_topology(&worker, tpg);
    } 
    else {
      //assert(f._topologies.size() == 1);

      // Need to back up the promise first here becuz taskflow might be 
      // destroy soon after calling get
      auto p {std::move(tpg->_promise)};

      // Back up lambda capture in case it has the topology pointer, 
      // to avoid it releasing on pop_front ahead of _mutex.unlock & 
      // _promise.set_value. Released safely when leaving scope.
      auto c {std::move(tpg->_call)};

      // Get the satellite if any
      auto s {f._satellite};
      
      // Now we remove the topology from this taskflow
      f._topologies.pop();

      //f._mutex.unlock();
      lock.unlock();
      
      // We set the promise in the end in case taskflow leaves the scope.
      // After set_value, the caller will return from wait
      p.set_value();

      _decrement_topology_and_notify();
      
      // remove the taskflow if it is managed by the executor
      // TODO: in the future, we may need to synchronize on wait 
      // (which means the following code should the moved before set_value)
      if(s) {
        std::scoped_lock<std::mutex> lock(_taskflow_mutex);
        _taskflows.erase(*s);
      } 
    }
  }
}

// ############################################################################
// Forward Declaration: Subflow
// ############################################################################

inline void Subflow::join() {

  if(!_joinable) {
    TF_THROW("subflow not joinable");
  }

  _executor._invoke_dynamic_task_external(_parent, _graph, false);
  _joinable = false;
}

inline void Subflow::detach() {

  if(!_joinable) {
    TF_THROW("subflow already joined or detached");
  }

  _executor._invoke_dynamic_task_external(_parent, _graph, true);
  _joinable = false;
}

// Function: named_async
template <typename F, typename... ArgsT>
auto Subflow::named_async(const std::string& name, F&& f, ArgsT&&... args) {

  _parent->_join_counter.fetch_add(1);

  using T = std::invoke_result_t<F, ArgsT...>;
  using R = std::conditional_t<std::is_same_v<T, void>, void, std::optional<T>>;

  std::promise<R> p;

  auto tpg = std::make_shared<AsyncTopology>();

  Future<R> fu(p.get_future(), tpg);

  auto node = node_pool.animate(
    std::in_place_type_t<Node::Async>{},
    [p=make_moc(std::move(p)), f=std::forward<F>(f), args...] 
    (bool cancel) mutable {
      if constexpr(std::is_same_v<R, void>) {
        if(!cancel) {
          f(args...);
        }
        p.object.set_value();
      }
      else {
        p.object.set_value(cancel ? std::nullopt : std::make_optional(f(args...)));
      }
    },
    std::move(tpg)
  );
  
  node->_name = name;
  node->_topology = _parent->_topology;
  node->_parent = _parent;

  _executor._schedule(this_worker().worker, node);

  return fu;
}

// Function: async
template <typename F, typename... ArgsT>
auto Subflow::async(F&& f, ArgsT&&... args) {
  return named_async("", std::forward<F>(f), std::forward<ArgsT>(args)...);
}

// Function: silent_async
template <typename F, typename... ArgsT>
void Subflow::named_silent_async(const std::string& name, F&& f, ArgsT&&... args) {

  _parent->_join_counter.fetch_add(1);

  auto node = node_pool.animate(
    std::in_place_type_t<Node::SilentAsync>{},
    [f=std::forward<F>(f), args...] () mutable { 
      f(args...); 
    }
  );
  
  node->_name = name;
  node->_topology = _parent->_topology;
  node->_parent = _parent;
  
  _executor._schedule(this_worker().worker, node);
}

// Function: named_silent_async
template <typename F, typename... ArgsT>
void Subflow::silent_async(F&& f, ArgsT&&... args) {
  named_silent_async("", std::forward<F>(f), std::forward<ArgsT>(args)...);
}



}  // end of namespace tf -----------------------------------------------------

