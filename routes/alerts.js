const express = require('express');

const router = express.Router();

// GET alerts
router.get('/', (req, res, next) => {
    const alerts = [
        {
            level: 'warning',
            title: 'Warning test',
            content: 'Lorem ipsum',
        },
        {
            level: 'danger',
            title: 'Danger test',
            content: 'Lorem ipsum',
        },
    ];

    return res.render('alerts', {
        title: 'Alerts',
        description: 'Here you can see a list of system alerts.',
        alerts,
    });
});

module.exports = router;
