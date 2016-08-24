power_LoadTest does not need to pack the extension to run it, but it
can be packed to run it in your browser.

In order to update extension.crx, use chrome's built in packer. You must close
all chrome windows before running this command.

/opt/google/chrome/chrome --pack-extension=./extension \
  --pack-extension-key=./extension.pem --no-message-box


Alternatively, extension developer mode will provide a 
GUI way of doing the same task.

If running manually, click on the power_LoadTest extension icon to begin the
test with default settings (3600 second test).
