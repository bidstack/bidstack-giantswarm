#include <QString>

#include "giantswarmerror.hpp"

using namespace Bidstack::Giantswarm;

QString GiantswarmError::errorString() const {
    switch (error) {
        case InvalidJsonFromCache:
          return "Received invalid JSON from cache!";

        case InvalidJsonFromAPI:
          return "Received invalid JSON from API!";

        case NotAllowedToRequestURI:
          return "Not allowed to request given URI!";

        case ClientError:
          return "An client error occurred!";

        case ServerError:
          return "An server error occurred!";

        case ResponseContainsRedirection:
          return "Response contains unhandled redirection!";

        case NotFound:
          return "Requested URI not found!";

        case UnexpectedResponseStatus:
          return "Unexpected response status!";

        case LoginRequired:
          return "Login required!";

        case LogoutRequired:
          return "Logout required!";

        case ResponseStatusMismatch:
          return "Received status_code does not match expected status!";
    }

    return QString();
}
