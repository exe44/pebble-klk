"use strict";

// -----------------------------------------------------------------------------

function sendToPebble(msg) {
  Pebble.sendAppMessage(msg,
    function(e) {
      console.log("Successfully delivered message with transactionId: " + e.data.transactionId);
    },
    function(e) {
      console.log("Unable to deliver message with transactionId: " + e.data.transactionId + " Error is: " + e.error.message);
    }
  );
}

// -----------------------------------------------------------------------------

Pebble.addEventListener("ready", function(e) {
  console.log("JavaScript app ready and running! payload: " + JSON.stringify(e.payload));
});

// -----------------------------------------------------------------------------

Pebble.addEventListener("showConfiguration", function(e) {
  console.log("Show configuration! payload: " + JSON.stringify(e.payload));

  var configUrl = "https://exe44.github.io/pebble-klk/config-web/config-klk.html";

  var config_json = localStorage.getItem("config");
  if (config_json === null) {
    config_json = "{}";
  }
  var config = JSON.parse(config_json);

  var watch = Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;
  if(watch) {
    config.isPebbleColor = (watch.platform === "aplite") ? 0 : 1;
    config.isAplite = (watch.platform === "aplite") ? 1 : 0;
  }

  if (config) {
    console.log("stored config: " + JSON.stringify(config));

    var params = "";

    for (var key in config) {
      var value = config[key];
      console.log("key: " + key + " value: " + value);

      if (params.length > 0)
        params += "&";
      else
        params += "?";

      params += (key + "=" + encodeURIComponent(value));
    }

    configUrl += params;
  }

  console.log("Opening Url: " + configUrl);
  Pebble.openURL(configUrl);
});

// -----------------------------------------------------------------------------

Pebble.addEventListener("webviewclosed", function(e) {
  var response = decodeURIComponent(e.response);
  console.log("webviewclosed response: " + response);

  if (response.length > 0) {
    localStorage.setItem("config", response);
    sendToPebble(JSON.parse(response));
  }
});

// -----------------------------------------------------------------------------

Pebble.addEventListener("appmessage", function(e) {
  console.log("AppMessage received! payload: " + JSON.stringify(e.payload));
});
