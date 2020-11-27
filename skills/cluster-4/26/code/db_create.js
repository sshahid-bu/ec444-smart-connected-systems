var MongoClient = require('mongodb').MongoClient;
var url = 'mongodb://localhost:27017/smoke_db';

// create initial database
MongoClient.connect(url, (err, db) => {
  if (err) throw err;
  console.log('Database created.');
  db.close();
});