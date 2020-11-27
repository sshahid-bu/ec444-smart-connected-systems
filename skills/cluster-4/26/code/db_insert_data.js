var MongoClient = require('mongodb').MongoClient;
var url = 'mongodb://localhost:27017/';
const fs = require('fs');


populateDB = (db_obj) => {
  fs.readFile('./data/smoke.txt', 'utf8', (err, data) => {
    if (err) throw err;
    var data_lines = data.split('\n');

    for (i=1; i < data_lines.length-1; i++) {
      data_elems = data_lines[i].split('\t');
      var data_obj = {
        time: data_elems[0],
        sensorID: data_elems[1],
        smok: data_elems[2],
        temp: data_elems[3],
      };

      console.log(data_obj);
      console.log(`Wrote object ${i}`);

      db_obj.collection('smoke_data').insertOne(data_obj, (err, res) => {
        if (err) throw err;
      });
    }
    console.log('DB write complete.');
  });
};

MongoClient.connect(url, (err, db) => {
  if (err) throw err;
  var dbo = db.db('smoke_db');

  dbo.createCollection('smoke_data', (err, db) => {
    if (err) throw err;
    console.log('Collection created!');
    populateDB(dbo, () => {
      db.close();
    });
  });
});