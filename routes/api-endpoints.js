const express = require('express');
const api = require('../api');

const router = express.Router();

// GET data points
router.get('/data', (req, res, next) => {
    api.getReservoirDataPoints(req.query.id, req.query.quantity, (err, data) => {
        if (err) {
            return next(err);
        }

        return res.json(data);
    });
});

// POST data point
router.post('/data', (req, res, next) => {
    api.appendReservoirDataPoint(req.body.id, req.body.value, (err) => {
        if (err) {
            return next(err);
        }

        return res.sendStatus(200);
    });
});

// GET reservoir info
router.get('/info', (req, res, next) => {
    api.getReservoirInfo(req.query.id, (err, data) => {
        if (err) {
            return next(err);
        }

        return res.json(data);
    });
});

// POST reservoir info
router.post('/info', (req, res, next) => {
    const data = {
        name: req.body.name,
        max: req.body.max,
        min: req.body.min,
    };

    api.setReservoirInfo(req.body.id, data, (err) => {
        if (err) {
            return next(err);
        }

        return res.redirect('/');
    });
});

// DELETE reservoir
router.get('/delete', (req, res, next) => {
    api.deleteReservoir(req.query.id, (err) => {
        if (err) {
            return next(err);
        }

        return res.redirect('/');
    });
});

// GET alerts
router.get('/alerts', (req, res, next) => {
    api.getAlerts((err, resp) => {
        if (err) {
            return next(err);
        }

        return res.json(resp);
    });
});

module.exports = router;
