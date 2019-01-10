window.onload = statusRequest();
setInterval(statusRequest,2000);
function statusRequest() {
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {		
			if (this.responseText === null) {
				console.log("No Data in Response") 
			} else {
				var obj = JSON.parse(this.responseText)
				//Update Temperature
				document.getElementById("TempValue").innerHTML = obj.temperature;
				// Update Temp Class based on Heat On Off Status
				if (obj.relayStatus == 1) {
					//document.getElementById("temperature").className = "heatOn";
					document.getElementById("heatingText").style.display = "inherit";
					} else {
					//document.getElementById("temperature").className = "heatOff";
					document.getElementById("heatingText").style.display = "none";
				}	
				document.getElementById("lowTemp").innerHTML = obj.lowTemp;
				document.getElementById("highTemp").innerHTML = obj.highTemp;
			}
		}	
	};
xhttp.open("GET", "statusRequest", true);
xhttp.send();
}

//function submitForm() {document.tempUpdate.submit();}

window.addEventListener("load", function () {
  function sendData() {
    var XHR = new XMLHttpRequest();

    // Bind the FormData object and the form element
    var FD = new FormData(form);

    // Define what happens on successful data submission
    XHR.addEventListener("load", function(event) {
		var obj = JSON.parse(this.responseText)
		document.getElementById("lowTemp").innerHTML = obj.lowTemp;
		document.getElementById("highTemp").innerHTML = obj.highTemp;
		document.getElementById("tempUpdate").reset();
		//alert(event.target.responseText);
    });

    // Define what happens in case of error
    XHR.addEventListener("error", function(event) {
		alert('Oops! Something went wrong.');
    });

    // Set up our request
    XHR.open("POST", "/tempChange");

    // The data sent is what the user provided in the form
    XHR.send(FD);
  }
 
  // Access the form element...
  var form = document.getElementById("tempUpdate");

  // ...and take over its submit event.
  form.addEventListener("submit", function (event) {
    event.preventDefault();

    sendData();
  });
});