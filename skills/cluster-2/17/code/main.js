var http  = require('http');
var fs    = require('fs');
var url   = require('url');

http.createServer(function (req, res) {
  var q = url.parse(req.url, true);
  var filename = './' + q.pathname;

  fs.readFile(filename, function(err, data) {
    if (err) { return res.end("404 Not Found"); }
    res.write(data);
    return res.end();
  });
}).listen(8080);