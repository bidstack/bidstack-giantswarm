#ifndef BIDSTACK_GIANTSWARM_ERROR_HPP
#define BIDSTACK_GIANTSWARM_ERROR_HPP

namespace Bidstack {
    namespace Giantswarm {

        class GiantswarmError {
        public:
            enum Error {
                InvalidJsonFromCache = 0,
                InvalidJsonFromAPI = 1,
                NotAllowedToRequestURI = 2,
                ClientError = 3,
                ServerError = 4,
                ResponseContainsRedirection = 5,
                NotFound = 6,
                UnexpectedResponseStatus = 7,
                LoginRequired = 8,
                LogoutRequired = 9,
                ResponseStatusMismatch = 10
            };

        public:
            QString errorString() const;

        public:
            Error error;
        };

    };
};

#endif
