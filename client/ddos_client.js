var http = require('http');

// Initialise parameters
var clients = process.argv[2];
var max_timeout = process.argv[3];
var client_options = [];
var stopped = 0;
var running_clients = 0;


// Show usage message if parameters lacking
if (process.argv.length < 4 ||
    isNaN(clients) || clients <= 0 ||
    isNaN(max_timeout) || max_timeout <= 0) {
    
    console.log("usage: node " + process.argv[1] + " <clients-count> <max-timeout-ms>");
    process.exit(0);
}


// This is just the responce printing handler
callback = function(response) {
    var str = '';
    //another chunk of data has been recieved, so append it to `str`
    response.on('data', function (chunk) {
        str += chunk;
    });

    //the whole response has been recieved, so we just print it out here
    response.on('end', function () {
        console.log(str);
    });
}



// Init the client options for each client
for (i = 0; i < clients; i++) {
    client_options[i] = {
        host: 'localhost',
        port: 8080,
        path: '/?client_id=' + i
    };
}


// send request and load random timeout
function send_request(client_id) {
    http.request(client_options[client_id], callback).end();
    var timeout = Math.floor((Math.random() * max_timeout));
    setTimeout(send_request, timeout, client_id);
}

// Start the clients with random timeout!
for (i = 0; i < clients; i++) {
    var timeout = Math.floor((Math.random() * max_timeout));
    running_clients++;
    setTimeout(send_request, timeout, i);
}


// Get any keystroke
var stdin = process.openStdin();
process.stdin.setRawMode(true);
stdin.on('data', function (text) {
    console.log("Bye...");
    process.exit(0);
});
