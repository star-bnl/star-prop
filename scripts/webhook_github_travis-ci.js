var incomingPars = JSON.parse(request.body);

var outcomingPars = {
  'request': {
    'branch': 'ds-travis-ci',
    'config': {
      'env': {
        'STAR_CVS_REF': incomingPars['after']
      }
    }
  }
};

request.body = outcomingPars;

request.headers['Content-Type']       = 'application/json';
request.headers['Accept']             = 'application/json';
request.headers['Travis-API-Version'] = '3';
request.headers['Authorization']      = 'token ' + incomingPars['token'];

// Forward a request. For example, test it with the following:
// curl -s -H "Content-Type: application/json" -d '{"after": "SL19e", "token": "***"}' -X POST https://putsreq.com/wl0MU9JUBEqQusYEToJJ
request.forwardTo = 'https://putsreq.com/u10VuM9cKQSyMYbClS0F';
//request.forwardTo = 'https://api.travis-ci.org/repo/star-bnl%2Fstar-sw/requests';
