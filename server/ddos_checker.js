
var url = require('url');




module.exports = {
    check: function(request) {
        var url_parts = url.parse(request.url, true);
        var client_id = url_parts.query.client_id;
        console.log(JSON.stringify(url_parts));
        console.log("client " + client_id);
        return (client_id < 4);
    }
};