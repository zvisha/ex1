/**
 * Created by zvisha on 22/10/2015.
 */

//Lets require/import the HTTP module
var http = require('http');
var url = require('url');

//Lets define a port we want to listen to
const PORT=8080;


function ddos_check_path(request) {
    var url_parts = url.parse(request.url, true);
    var client_id = url_parts.query.client_id;
    console.log(JSON.stringify(url_parts));
    console.log("client " + client_id);
    return (client_id < 4);
}


//We need a function which handles requests and send response
function handleRequest(request, response){
    if (ddos_check_path(request)) {
        response.end('It Works!! Path Hit: ' + request.url);
    } else {
        response.writeHead(503, {"Content-Type": "text/html"});
        response.write('Service Unavailable');
        response.end();
    }
}

//Create a server
var server = http.createServer(handleRequest);

//Lets start our server
server.listen(PORT, function(){
    //Callback triggered when server is successfully listening. Hurray!
    console.log("Server listening on: http://localhost:%s", PORT);
});


