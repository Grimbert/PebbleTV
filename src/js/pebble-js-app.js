var user = localStorage.getItem("trakt_username");
var type = localStorage.getItem("trakt_24h");

if (!user) {
  user = "justin";
}

if(!type){
	type = 1;
}

function fetchShows(symbol) {
  var response;
  var req = new XMLHttpRequest();
  
  var param = new Date();
  if(user == null || user == "CANCELLED" || user.length < 1){
		user = 'justin';
  }
  req.open('GET', 'http://api.trakt.tv/user/calendar/shows.json/8992c1aed8ebaa6fd0c9dc9a312363a3/'+user+"/"+param.getDate()+"-"+(param.getMonth()+1)+"-"+param.getFullYear()+"/61"
    , true);
  req.onload = function(e) {
    if (req.readyState == 4) {
     
    	if(req.status == 200) {
			var response = JSON.parse(req.responseText);
			var max = response.length;
			if(max > 8){
				max = 8;
			}
			var shows = "";  
			var shows2 = "";
			var counter = 0;
			for (var i = 0; i < max; i++) {
				var c = response[i];  					
				var  episodes = c["episodes"];
				var ep_date = c["date"];
				var day = ep_date.substring(ep_date.length-2);
				var month = ep_date.substring(ep_date.length-5,ep_date.length-3);
									for(var a = 0; a < episodes.length; a++){
										if (counter < max){
											var  d = episodes[a];
											var  e = d["show"];
											var  title = e["title"];
											if(title.length > 19){
														title = title.substring(0, 18)+"...";
													}
											if(counter > 3){
												if(type == 1){
													shows2 += day+"-"+month+": "+title;
												}else{
													shows2 += month+"-"+day+": "+title;
												}
												if(counter < 8){
													shows2 += "\n";
												}
											}else{
												if(type == 1){
													shows += day+"-"+month+": "+title;
													
												}else{
													shows += month+"-"+day+": "+title;
												}
												if(counter < 3){
													shows += "\n";
												}
											}
											
											counter++;
										}
									}
			}
			  
			if (max > 4){
				Pebble.sendAppMessage({"0": shows, "1": shows2});
			}else{
				Pebble.sendAppMessage({"0": shows});
			}
		}else{
			Pebble.showSimpleNotificationOnPebble("PebbleTV", "Could not update TV shows for user "+user);
		}
	}else{
		Pebble.showSimpleNotificationOnPebble("PebbleTV", "Could not update TV shows for user "+user);
	}
  }
  req.send(null);
}


Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          console.log(e.type);
						  Pebble.sendAppMessage({"2": "refresh"});
                        });

Pebble.addEventListener("showConfiguration",
		function(e) {
			Pebble.openURL("http://bertderuiter.nl/downloads/pebbletv.php?user="+user);
	}
);


Pebble.addEventListener("webviewclosed",
  function(e) {
      console.log("Configuration window returned: " + e.response);
	  
	  if(e.response == "CANCELLED"){
		  fetchShows();
		  return;
	  }
	  user = e.response;
	  localStorage.setItem("trakt_username", user);
	  fetchShows();
  }
);


Pebble.addEventListener("appmessage",
                        function(e) {
                          console.log("message");
							console.log("Received message: " + e.payload[1]);
							localStorage.setItem("trakt_24h", e.payload[1]);
							type = e.payload[1];
							fetchShows();
                         
                        });
