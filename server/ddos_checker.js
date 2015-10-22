
var url = require('url');

var access_period    = 5000;
var max_access_count = 5;

// Datastructure to hold all clients
var latest_clients = {};

function try_clean_up_entry(client_id, time_now) {
    var data = latest_clients[client_id];
    var first_access = data.first_access;
    if (time_now - first_access >= access_period) {
        delete latest_clients[client_id];
        return true;
    }
    return false;
}

// clean old entries in array
function clean_up() {
    var time_now = Date.now();
    for (var client_id in latest_clients) {
        try_clean_up_entry(client_id, time_now);
    }
}



// Update access and return if OK to reply
function client_access_ok(client_id) {
    if (client_id in latest_clients) {
        if (try_clean_up_entry(client_id, Date.now())) {
            // Small recursion but never deeper then 2, so Ok.
            return client_access_ok(client_id);
        }

        var data = latest_clients[client_id];
        data.count++;
        if (data.count > max_access_count) {
            return false;
        }
    } else {
        latest_clients[client_id] = {
            count:        1,
            first_access: Date.now()
        }
    }
    return true;
}

// Clean old entries every 5 seconds to  clean junk memory
setInterval(clean_up, 5000);


module.exports = {
    check: function(request) {
        var url_parts = url.parse(request.url, true);
        var client_id = url_parts.query.client_id;
        return client_access_ok(client_id);
    }
};