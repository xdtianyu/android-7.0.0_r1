For system daemons which need to interface with the weave daemon (weaved), these
daemons will need to link to **libweaved**.

The `weaved::Service` class is an entry point into weave daemon interface.
This class maintains an IPC connection to the daemon and allows clients to
register weave command handlers and update the device state they are
responsible for.

In order to create an instance of `Service`, call asynchronous
`Service::Connect` static method. This method initiates a connection to weaved
and once established invokes the provided `callback`. When the callback is
invoked, the connection to the weave daemon is available and the client should
create their component, register command handlers and update the state.
If connection is lost (e.g. the weave daemon exist), the provided weak
pointer to the `Service` object becomes invalidated. As soon as weaved is
restarted and the connection is restored, the `callback` is invoked again and
the client can re-register command handlers, update the state again.

A simple client daemon that works with weaved could be as follows:

```
class Daemon final : public brillo::Daemon {
 public:
  Daemon() = default;

 protected:
  int OnInit() override;

 private:
  void OnConnected(const std::weak_ptr<weaved::Service>& service);
  void OnCommand1(std::unique_ptr<weaved::Command> command);
  void UpdateDeviceState();

  std::unique_ptr<weaved::Service::Token> weave_service_token_;
  std::weak_ptr<weaved::Service> weave_service_;
  brillo::BinderWatcher binder_watcher_;
  base::WeakPtrFactory<Daemon> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(Daemon);
};

int Daemon::OnInit() {
  android::BinderWrapper::Create();
  if (!binder_watcher_.Init())
    return EX_OSERR;

  weave_service_token_ = weaved::Service::Connect(
      brillo::MessageLoop::current(),
      base::Bind(&Daemon::OnConnected, weak_ptr_factory_.GetWeakPtr()));
  return brillo::Daemon::OnInit();
}

void Daemon::OnConnected(const std::weak_ptr<weaved::Service>& service) {
  weave_service_ = service;
  auto weave_service = weave_service_.lock();
  if (!weave_service)
    return;

  weave_service->AddComponent("myComponent", {"_myTrait"}, nullptr);
  weave_service->AddCommandHandler(
      "myComponent", "_myTrait.command1",
      base::Bind(&Daemon::OnCommand1, base::Unretained(this)));
  UpdateDeviceState();
}

void Daemon::UpdateDeviceState() {
  auto weave_service = weave_service_.lock();
  if (!weave_service)
    return;

  brillo::VariantDictionary state_change{
    {"_myTrait.state1", 12},
    {"_myTrait.state2", std::string{"foo"}},
  };
  weave_service->SetStateProperties("myComponent", state_change, nullptr);
}
```