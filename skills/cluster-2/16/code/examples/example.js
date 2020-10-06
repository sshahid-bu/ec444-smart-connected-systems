var http    = require('http');
var dt      = require('./dt');
var url     = require('url');
var uc      = require('upper-case');
var events  = require('events');

http.createServer(function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write("Date and Time are currently: " + dt.myDateTime());
    res.write(uc.upperCase("\n\nHello World\n"));
}).listen(8080);

var eventEmitter = new events.EventEmitter();
var myEventHandler = function () {
    console.log('I hear a scream!');
}
eventEmitter.on('scream', myEventHandler);

var adr = 'http://localhost:8080/default.htm?year=2020&month=october';
var q = url.parse(adr, true);

console.log(q.host);
console.log(q.pathname);
console.log(q.search);

var qdata = q.query;
console.log(qdata.month);

eventEmitter.emit('scream');