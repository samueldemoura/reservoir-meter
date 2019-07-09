const moment = require('moment');
const db = require('./db');

/**
 * For these first two functions, "data" is an object containing:
 * - name: Name of the reservoir.
 * - max: Maximum water level (in meters).
 * - min: Minimum water level before firing off an alert (in meters).
 */

module.exports = {
    /**
     * Fetches information about a specific reservoir.
     * @param {*} id Reservoir identifier.
     */
    getReservoirInfo(id, callback) {
        db.hgetall(`${id}_info`, (err, resp) => callback(err, resp));
    },

    /**
     * Sets or updates information about a specific reservoir.
     * @param {*} id Reservoir identifier.
     * @param {*} data New data.
     */
    setReservoirInfo(id, data, callback) {
        db.hmset(`${id}_info`, data, (err, resp) => {
            if (callback) callback(err, resp);
        });
    },

    /**
     * Returns list of all reservoir identifiers in the database.
     */
    getReservoirIds(callback) {
        db.scan(0, 'match', '*_info', (err, resp) => {
            const reservoirs = [];

            for (let i = 0; i < resp[1].length; i += 1) {
                const id = resp[1][i].slice(0, -5); // slice off "_info"
                reservoirs.push(id);
            }

            return callback(err, reservoirs);
        });
    },

    /**
     * Returns list of all reservoirs in the database.
     */
    getReservoirList(callback) {
        this.getReservoirIds(async (err, resp) => {
            const promises = resp.map(async () => {
                const info = await db.hgetallAsync(`${resp}_info`);
                return {
                    id: resp[0],
                    name: info.name,
                    max: info.max,
                };
            });

            const reservoirs = await Promise.all(promises);
            callback(err, reservoirs);
        });
    },

    /**
     * Fetches logged data points for a specific reservoir.
     * @param {*} id Reservoir identifier.
     * @param {*} quantity Number of data points to fetch.
     */
    getReservoirDataPoints(id, quantity, callback) {
        db.lrange(`${id}_log`, -quantity, -1, (err, resp) => callback(err, resp));
    },

    /**
     * Appends a data point to the specified reservoir's log.
     * @param {*} id Reservoir identifier.
     * @param {*} value Raw measurement value.
     */
    appendReservoirDataPoint(id, value, callback) {
        // Create reservoir in database if it doesn't exist
        this.getReservoirInfo(id, (err, resp) => {
            if (err) {
                throw err;
            }

            if (!resp) {
                this.setReservoirInfo(id, {
                    name: `Reservoir #${id}`,
                    max: 4.5,
                });
            }
        });

        // Append data
        const now = moment();
        const data = [now, value];
        db.rpush(`${id}_log`, JSON.stringify(data), (err, resp) => callback(err, resp));
    },

    /**
     * Deletes a reservoir and its history from the database.
     * @param {*} id Reservoir identifier.
     */
    deleteReservoir(id, callback) {
        db.del(`${id}_log`, (err, resp) => {
            if (err) {
                return callback(err, resp);
            }

            return db.del(`${id}_info`, (err2, resp2) => callback(err2, resp2));
        });
    },
};
