window.onload = statusRequest();
setInterval(statusRequest,30000);
function statusRequest() {
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {		
			if (this.responseText === null) {
				console.log("No Data in Response") 
			} else {
				var obj = JSON.parse(this.responseText)
				document.getElementById("TempValue").innerHTML = obj.temperature;
				
				if (obj.relayStatus == 1) {
					document.getElementById("temperature").className = "heatOn";
					} else {
					document.getElementById("temperature").className = "heatOff";
				}	
			}
		}	
	};
xhttp.open("GET", "statusRequest", true);
xhttp.send();
}

function submitForm() {document.tempUpdate.submit();}