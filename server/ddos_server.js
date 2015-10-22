/**
 * Created by zvisha on 22/10/2015.
 */

//Lets require/import the HTTP module
var http = require('http');
var ddos = require('./ddos_checker.js');

//Lets define a port we want to listen to
const PORT=8080;


//We need a function which handles requests and send response
function handleRequest(request, response){
    if (ddos.check(request)) {
        response.end("OK");
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
    console.log("Server listening on: http://localhost:%s", PORT);
});



// Get any keystroke
var stdin = process.openStdin();
process.stdin.setRawMode(true);
stdin.on('data', function (text) {
    console.log("Gracefully exit");
    process.exit(0);
});
