const util = require('util');
const redis = require('redis');

const client = redis.createClient();

client.hgetallAsync = util.promisify(client.hgetall).bind(client);

// TODO: Proper logging
client.on('connect', () => {
    // eslint-disable-next-line no-console
    console.log('Redis client connected');
});

client.on('error', (e) => {
    // eslint-disable-next-line no-console
    console.error(`Redis client error: ${e}`);
});

module.exports = client;
