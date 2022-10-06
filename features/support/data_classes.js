'use strict';

const util = require('util');

module.exports = {
    Location: class {
        constructor (lon, lat) {
            this.lon = lon;
            this.lat = lat;
        }
    },

    FuzzyMatch: class {
        match (got, want) {
            // don't fail if bearings input and extected string is empty and actual result is undefined
            if (want === '' && (got === '' || got === undefined))
                return true;

            var matchPercent = want.match(/(.*)\s+~(.+)%$/),
                matchAbs = want.match(/(.*)\s+\+-(.+)$/),
                matchRe = want.match(/^\/(.*)\/$/),
                // we use this for matching before/after bearing
                matchBearingListAbs = want.match(/^((\d+)->(\d+))(,(\d+)->(\d+))*\s+\+-(.+)$/),
                matchIntersectionListAbs = want.match(/^(((((true|false):\d+)\s{0,1})+,{0,1})+;{0,1})+\s+\+-(.+)$/),
                matchRangeNumbers = want.match(/\d+\+-\d+/);

            function inRange(margin, got, want) {
                var fromR = parseFloat(want) - margin,
                    toR = parseFloat(want) + margin;
                return parseFloat(got) >= fromR && parseFloat(got) <= toR;
            }
            function parseIntersectionString(str) {
                return str.split(';')
                    .map((turn_intersections) => turn_intersections
                        .split(',')
                        .map((intersection) => intersection
                            .split(' ')
                            .map((entry_bearing_pair) => entry_bearing_pair
                                .split(':'))));
            }

            if (got === want) {
                return true;
            } else if (matchBearingListAbs) {
                let want_and_margin = want.split('+-'),
                    margin = parseFloat(want_and_margin[1].trim()),
                    want_pairs = want_and_margin[0].trim().split(',').map((pair) => pair.split('->')),
                    got_pairs = got.split(',').map((pair) => pair.split('->'));
                if (want_pairs.length != got_pairs.length)
                {
                    return false;
                }
                for (var i = 0; i < want_pairs.length; ++i)
                {
                    if (!inRange(margin, got_pairs[i][0], want_pairs[i][0]) ||
                        !inRange(margin, got_pairs[i][1], want_pairs[i][1]))
                    {
                        return false;
                    }
                }
                return true;
            } else if (matchIntersectionListAbs) {
                let margin = parseFloat(want.split('+-')[1]),
                    want_intersections = parseIntersectionString(want.split('+-')[0].trim()),
                    got_intersections = parseIntersectionString(got);
                if (want_intersections.length != got_intersections.length)
                {
                    return false;
                }
                for (let step_idx = 0; step_idx < want_intersections.length; ++step_idx)
                {
                    if (want_intersections[step_idx].length != got_intersections[step_idx].length)
                    {
                        return false;
                    }
                    for (let intersection_idx = 0; intersection_idx < want_intersections[step_idx].length; ++intersection_idx)
                    {
                        if (want_intersections[step_idx][intersection_idx].length != got_intersections[step_idx][intersection_idx].length)
                        {
                            return false;
                        }
                        for (let pair_idx = 0; pair_idx < want_intersections[step_idx][intersection_idx].length; ++pair_idx)
                        {
                            let want_pair = want_intersections[step_idx][intersection_idx][pair_idx],
                                got_pair = got_intersections[step_idx][intersection_idx][pair_idx];
                            if (got_pair[0] != want_pair[0] ||
                                !inRange(margin, got_pair[1], want_pair[1]))
                            {
                                return false;
                            }
                        }
                    }
                }
                return true;
            } else if (matchPercent) {         // percentage range: 100 ~ 5%
                var target = parseFloat(matchPercent[1]),
                    percentage = parseFloat(matchPercent[2]);
                if (target === 0) {
                    return true;
                } else {
                    let ratio = Math.abs(1 - parseFloat(got) / target);
                    return 100 * ratio < percentage;
                }
            } else if (matchAbs) {             // absolute range: 100 +-5
                let margin = parseFloat(matchAbs[2]);
                return inRange(margin, got, matchAbs[1]);
            } else if (matchRe) {               // regex: /a,b,.*/
                return got.match(matchRe[1]);
            } else if (matchRangeNumbers) {
                let real_want_and_margin = want.split('+-'),
                    margin = parseFloat(real_want_and_margin[1].trim()),
                    real_want = parseFloat(real_want_and_margin[0].trim());
                return inRange(margin, got, real_want);
            } else {
                return false;
            }
        }

        matchLocation (got, want) {
            if (got == null || want == null) return false;
            return this.match(got[0], util.format('%d ~0.0025%', want.lon)) &&
                this.match(got[1], util.format('%d ~0.0025%', want.lat));
        }

        matchCoordinate (got, want, zoom) {
            if (got == null || want == null) return false;
            return this.match(got.lon, util.format('%d +- %d', want.lon, 0.25*zoom)) &&
                this.match(got.lat, util.format('%d +- %d', want.lat, 0.25*zoom));
        }

    }
};
