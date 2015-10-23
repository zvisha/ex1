/*
   Node.js webserver with DDOS protection.
 */

//Lets require/import the HTTP module
var http = require('http');
var ddos = require('./ddos_checker.js');
var port = process.argv[2] || 8080;



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
server.listen(port, function(){
    console.log("Server listening on: http://localhost:%s", port);
});



// Get any keystroke
var stdin = process.openStdin();
process.stdin.setRawMode(true);
stdin.on('data', function (text) {
    console.log("Gracefully exit");
    process.exit(0);
});
