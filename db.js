/* eslint-disable no-console */
const util = require('util');
const redis = require('redis');

const client = redis.createClient();

client.hgetallAsync = util.promisify(client.hgetall).bind(client);

// TODO: Proper logging
client.on('connect', () => {
  console.log('Redis client connected');
});

client.on('error', (e) => {
  if (e.toString().indexOf('ECONNREFUSED') !== -1) {
    console.error('Failed to connect to redis. Is the server running?');
    process.exit(1);
  }

  console.error(`Redis client error: ${e}`);
});

module.exports = client;
