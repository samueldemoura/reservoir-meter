const express = require('express');
const api = require('../api');

const router = express.Router();

// GET dashboard
router.get('/', (req, res, next) => {
  api.getReservoirList((err, reservoirs) => {
    if (err) {
      return next(err);
    }

    return res.render('dashboard', {
      title: 'Dashboard',
      description: 'Here you can see an overview of all reservoirs.',
      reservoirs,
    });
  });
});

module.exports = router;
