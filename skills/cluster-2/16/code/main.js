const SerialPort = require('serialport');
const ReadLine = require('@serialport/parser-readline');
const port = new SerialPort('/dev/ttyS3', { baudRate: 115200 });

const parser = new ReadLine();
port.pipe(parser);

parser.on('data', line => console.log(line))
port.write('POWER ON\n');
