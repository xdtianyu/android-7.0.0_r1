# Overview

The wrapper implements OS dependent services for libweave

# Building

### Build daemon examples

The example binaries land in the out/Debug/ directory build all of them at once:

```
make all-examples
```

...or one at a time.

```
make out/Debug/weave_daemon_light
```

# Prepare Host OS

### Enable user-service-publishing in avahi daemon
Set disable-user-service-publishing=no in /etc/avahi/avahi-daemon.conf

#### restart avahi
```
sudo service avahi-daemon restart
```

# Control device with the cloud

### Generate registration ticket
- Go to [OAuth 2.0 Playground](https://developers.google.com/oauthplayground/)
- "Step 1": Paste https://www.googleapis.com/auth/clouddevices and click to "Authorize APIs"
- "Step 2": Click "Exchange authorization code for tokens"
- "Step 3": Fill the form:
    * HTTP Method: POST
    * Request URI: https://www.googleapis.com/weave/v1/registrationTickets
    * Enter request body: ```{"userEmail": "me"}```
    * Click "Send the request", a ticket id will be returned in

```
            {
              "userEmail": "user@google.com",
              "kind": "weave#registrationTicket",
              "expirationTimeMs": "1443204934855",
              "deviceId": "0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d",
              "creationTimeMs": "1443204694855",
              "id": "93019287-6b26-04a0-22ee-d55ad23a4226"
            }
```
- Note: the ticket "id" is not used within 240 sec, it will be expired.


### Register device to cloud

- Copy the ticket "id" generated above: ```93019287-6b26-04a0-22ee-d55ad23a4226```
- Go to terminal, register and start the daemon with

```
        sudo out/Debug/weave_daemon_sample --registration_ticket=93019287-6b26-04a0-22ee-d55ad23a4226
```

- See something like:

```
        Publishing service
        Saving settings to /var/lib/weave/weave_settings.json
```

- Note: in second and future runs, --registration_ticket options is not necessary anymore
- Get your device id with

```
        sudo cat /var/lib/weave/weave_settings.json
```

- See something like:

```
        ...
        "device_id": 0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d
        ...
```

- Use this device_id for future communication with your device. It does not expire.
- Verify device is up with Weave Device Managers on
[Android](https://play.google.com/apps/testing/com.google.android.apps.weave.management),
[Chrome](https://chrome.google.com/webstore/detail/weave-device-manager/pcdgflbjckpjmlofgopidgdfonmnodfm)
or [Weave Developpers Console](https://weave.google.com/console/)

### Send Command to the Daemon

- Go to [OAuth 2.0 Playground](https://developers.google.com/oauthplayground/)
- "Step 1": Paste https://www.googleapis.com/auth/clouddevices and click to "Authorize APIs"
- "Step 2": Click "Exchange authorization code for tokens"
- "Step 3": Fill the form:
    * HTTP Method: POST
    * Request URI: https://www.googleapis.com/weave/v1/commands
    * Enter request body:

```
        {
          "deviceId": "0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d",
          "name": "_sample.hello",
          "component": "sample",
          "parameters": { "name": "cloud user" }
        }

```

- "Send the request", you command will be "queued" as its "state"
- Verify the command execution observing daemon console logs
- Verify the command usign [Weave Developpers Console](https://weave.google.com/console/)
- Verify the command history with [OAuth 2.0 Playground](https://developers.google.com/oauthplayground/)
- "Step 1": Paste https://www.googleapis.com/auth/clouddevices and click to "Authorize APIs"
- "Step 2": Click "Exchange authorization code for tokens"
- "Step 3": Fill the form:
    * HTTP Method: GET
    * Request URI: https://www.googleapis.com/weave/v1/commands?deviceId=0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d
    * Click "Send the request", you get all of the commands executed on your device.
