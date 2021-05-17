#pragma once

#include <chrono>
#include <functional>

#include "envoy/common/callback.h"
#include "envoy/common/time.h"
#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/event/dispatcher.h"
#include "envoy/event/timer.h"
#include "envoy/network/drain_decision.h"
#include "envoy/server/drain_manager.h"
#include "envoy/server/instance.h"

#include "common/common/callback_impl.h"
#include "common/common/logger.h"

namespace Envoy {
namespace Server {

/**
 * Implementation of drain manager that does the following by default:
 * 1) Terminates the parent process after 15 minutes.
 * 2) Drains the parent process over a period of 10 minutes where drain close becomes more
 *    likely each second that passes.
 */
class DrainManagerImpl : Logger::Loggable<Logger::Id::main>, public DrainManager {
public:
  DrainManagerImpl(Instance& server, envoy::config::listener::v3::Listener::DrainType drain_type);
  DrainManagerImpl(Instance& server, envoy::config::listener::v3::Listener::DrainType drain_type,
                   Event::Dispatcher& dispatcher);

  // Network::DrainDecision
  bool drainClose() const override;
  Common::ThreadSafeCallbackHandlePtr addOnDrainCloseCb(Event::Dispatcher& dispatcher,
                                                        DrainCloseCb cb) const override;

  // Server::DrainManager
  void startDrainSequence(std::function<void()> drain_complete_cb) override;
  bool draining() const override { return draining_; }
  void startParentShutdownSequence() override;
  DrainManagerSharedPtr
  createChildManager(Event::Dispatcher& dispatcher,
                     envoy::config::listener::v3::Listener::DrainType drain_type) override;
  DrainManagerSharedPtr createChildManager(Event::Dispatcher& dispatcher) override;

private:
  Instance& server_;
  const envoy::config::listener::v3::Listener::DrainType drain_type_;

  std::atomic<bool> draining_{false};
  Event::TimerPtr drain_tick_timer_;
  MonotonicTime drain_deadline_;
  mutable Common::ThreadSafeCallbackManager<std::chrono::milliseconds> cbs_;
  std::vector<Common::ThreadSafeCallbackHandlePtr> child_drain_cbs_;

  Event::TimerPtr parent_shutdown_timer_;
};

} // namespace Server
} // namespace Envoy
