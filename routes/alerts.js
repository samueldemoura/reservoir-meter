const express = require('express');
const api = require('../api');

const router = express.Router();

// GET alerts
router.get('/', (req, res, next) => {
  api.getAlertList((err, alerts) => {
    if (err) {
      return next(err);
    }

    const alertsWithMessages = alerts.map((alert) => {
      if (!alert) {
        return false;
      }

      return {
        title: `Reservoir ${alert.name} is low`,
        content: `Last measure of reservoir ${alert.name} is ${alert.level}m (below minimum value of ${alert.min}m).`,
      };
    }).filter(alert => alert); // filter out 'false' values (not actually an alert) from array

    return res.render('alerts', {
      title: 'Alerts',
      description: 'Here you can see a list of system alerts.',
      alerts: alertsWithMessages,
    });
  });
});

module.exports = router;
