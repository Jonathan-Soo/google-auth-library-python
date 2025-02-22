User Guide
==========

.. currentmodule:: google.auth

Credentials and account types
-----------------------------

:class:`~credentials.Credentials` are the means of identifying an application or
user to a service or API. Credentials can be obtained with three different types
of accounts: *service accounts*, *user accounts* and *external accounts*.

Credentials from service accounts identify a particular application. These types
of credentials are used in server-to-server use cases, such as accessing a
database. This library primarily focuses on service account credentials.

Credentials from user accounts are obtained by asking the user to authorize
access to their data. These types of credentials are used in cases where your
application needs access to a user's data in another service, such as accessing
a user's documents in Google Drive. This library provides no support for
obtaining user credentials, but does provide limited support for using user
credentials.

Credentials from external accounts (workload identity federation) are used to
identify a particular application from an on-prem or non-Google Cloud platform
including Amazon Web Services (AWS), Microsoft Azure or any identity provider
that supports OpenID Connect (OIDC).

Obtaining credentials
---------------------

.. _application-default:

Application default credentials
+++++++++++++++++++++++++++++++

`Google Application Default Credentials`_ abstracts authentication across the
different Google Cloud Platform hosting environments. When running on any Google
Cloud hosting environment or when running locally with the `Google Cloud SDK`_
installed, :func:`default` can automatically determine the credentials from the
environment::

    import google.auth

    credentials, project = google.auth.default()

If your application requires specific scopes::

    credentials, project = google.auth.default(
        scopes=['https://www.googleapis.com/auth/cloud-platform'])

Application Default Credentials also support workload identity federation to
access Google Cloud resources from non-Google Cloud platforms including Amazon
Web Services (AWS), Microsoft Azure or any identity provider that supports
OpenID Connect (OIDC). Workload identity federation is recommended for
non-Google Cloud environments as it avoids the need to download, manage and
store service account private keys locally.

.. _Google Application Default Credentials:
    https://developers.google.com/identity/protocols/
    application-default-credentials
.. _Google Cloud SDK: https://cloud.google.com/sdk


Service account private key files
+++++++++++++++++++++++++++++++++

A service account private key file can be used to obtain credentials for a
service account. You can create a private key using the `Credentials page of the
Google Cloud Console`_. Once you have a private key you can either obtain
credentials one of three ways:

1. Set the ``GOOGLE_APPLICATION_CREDENTIALS`` environment variable to the full
   path to your service account private key file

   .. code-block:: bash

        $ export GOOGLE_APPLICATION_CREDENTIALS=/path/to/key.json

   Then, use :ref:`application default credentials <application-default>`.
   :func:`default` checks for the ``GOOGLE_APPLICATION_CREDENTIALS``
   environment variable before all other checks, so this will always use the
   credentials you explicitly specify.

2. Use :meth:`service_account.Credentials.from_service_account_file
   <google.oauth2.service_account.Credentials.from_service_account_file>`::

        from google.oauth2 import service_account

        credentials = service_account.Credentials.from_service_account_file(
            '/path/to/key.json')

        scoped_credentials = credentials.with_scopes(
            ['https://www.googleapis.com/auth/cloud-platform'])

3. Use :meth:`service_account.Credentials.from_service_account_info
   <google.oauth2.service_account.Credentials.from_service_account_info>`::

        import json

        from google.oauth2 import service_account

        json_acct_info = json.loads(function_to_get_json_creds())
        credentials = service_account.Credentials.from_service_account_info(
            json_acct_info)

        scoped_credentials = credentials.with_scopes(
            ['https://www.googleapis.com/auth/cloud-platform'])

.. warning:: Private keys must be kept secret. If you expose your private key it
    is recommended to revoke it immediately from the Google Cloud Console.

.. _Credentials page of the Google Cloud Console:
    https://console.cloud.google.com/apis/credentials

Compute Engine, Container Engine, and the App Engine flexible environment
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Applications running on `Compute Engine`_, `Container Engine`_, or the `App
Engine flexible environment`_ can obtain credentials provided by `Compute
Engine service accounts`_. When running on these platforms you can obtain
credentials for the service account one of two ways:

1. Use :ref:`application default credentials <application-default>`.
   :func:`default` will automatically detect if these credentials are available.

2. Use :class:`compute_engine.Credentials`::

        from google.auth import compute_engine

        credentials = compute_engine.Credentials()

.. _Compute Engine: https://cloud.google.com/compute
.. _Container Engine: https://cloud.google.com/container-engine
.. _App Engine flexible environment:
    https://cloud.google.com/appengine/docs/flexible/
.. _Compute Engine service accounts:
    https://cloud.google.com/compute/docs/access/service-accounts

The App Engine standard environment
+++++++++++++++++++++++++++++++++++

Applications running on the `App Engine standard environment`_ can obtain
credentials provided by the `App Engine App Identity API`_. You can obtain
credentials one of two ways:

1. Use :ref:`application default credentials <application-default>`.
   :func:`default` will automatically detect if these credentials are available.

2. Use :class:`app_engine.Credentials`::

        from google.auth import app_engine

        credentials = app_engine.Credentials()

In order to make authenticated requests in the App Engine environment using the
credentials and transports provided by this library, you need to follow a few
additional steps:

#. If you are using the :mod:`google.auth.transport.requests` transport, vendor
   in the `requests-toolbelt`_ library into your app, and enable the App Engine
   monkeypatch. Refer `App Engine documentation`_ for more details on this.
#. To make HTTPS calls, enable the ``ssl`` library for your app by adding the
   following configuration to the ``app.yaml`` file::

        libraries:
        - name: ssl
          version: latest

#. Enable billing for your App Engine project. Then enable socket support for
   your app. This can be achieved by setting an environment variable in the
   ``app.yaml`` file::

        env_variables:
          GAE_USE_SOCKETS_HTTPLIB : 'true'

.. _App Engine standard environment:
    https://cloud.google.com/appengine/docs/python
.. _App Engine App Identity API:
    https://cloud.google.com/appengine/docs/python/appidentity/
.. _requests-toolbelt:
    https://toolbelt.readthedocs.io/en/latest/
.. _App Engine documentation:
    https://cloud.google.com/appengine/docs/standard/python/issue-requests

User credentials
++++++++++++++++

User credentials are typically obtained via `OAuth 2.0`_. This library does not
provide any direct support for *obtaining* user credentials, however, you can
use user credentials with this library. You can use libraries such as
`oauthlib`_ to obtain the access token. After you have an access token, you
can create a :class:`google.oauth2.credentials.Credentials` instance::

    import google.oauth2.credentials

    credentials = google.oauth2.credentials.Credentials(
        'access_token')

If you obtain a refresh token, you can also specify the refresh token and token
URI to allow the credentials to be automatically refreshed::

    credentials = google.oauth2.credentials.Credentials(
        'access_token',
        refresh_token='refresh_token',
        token_uri='token_uri',
        client_id='client_id',
        client_secret='client_secret')


There is a separate library, `google-auth-oauthlib`_, that has some helpers
for integrating with `requests-oauthlib`_ to provide support for obtaining
user credentials. You can use
:func:`google_auth_oauthlib.helpers.credentials_from_session` to obtain
:class:`google.oauth2.credentials.Credentials` from a 
:class:`requests_oauthlib.OAuth2Session` as above::

    from google_auth_oauthlib.helpers import credentials_from_session

    google_auth_credentials = credentials_from_session(oauth2session)

You can also use :class:`google_auth_oauthlib.flow.Flow` to perform the OAuth
2.0 Authorization Grant Flow to obtain credentials using `requests-oauthlib`_.

.. _OAuth 2.0:
    https://developers.google.com/identity/protocols/OAuth2
.. _oauthlib:
    https://oauthlib.readthedocs.io/en/latest/
.. _google-auth-oauthlib:
    https://pypi.python.org/pypi/google-auth-oauthlib
.. _requests-oauthlib:
    https://requests-oauthlib.readthedocs.io/en/latest/

External credentials (Workload identity federation)
+++++++++++++++++++++++++++++++++++++++++++++++++++

Using workload identity federation, your application can access Google Cloud
resources from Amazon Web Services (AWS), Microsoft Azure or any identity
provider that supports OpenID Connect (OIDC).

Traditionally, applications running outside Google Cloud have used service
account keys to access Google Cloud resources. Using identity federation,
you can allow your workload to impersonate a service account.
This lets you access Google Cloud resources directly, eliminating the
maintenance and security burden associated with service account keys.

Accessing resources from AWS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to access Google Cloud resources from Amazon Web Services (AWS), the
following requirements are needed:

- A workload identity pool needs to be created.
- AWS needs to be added as an identity provider in the workload identity pool
  (The Google organization policy needs to allow federation from AWS).
- Permission to impersonate a service account needs to be granted to the
  external identity.
- A credential configuration file needs to be generated. Unlike service account
  credential files, the generated credential configuration file will only
  contain non-sensitive metadata to instruct the library on how to retrieve
  external subject tokens and exchange them for service account access tokens.
- If you want to use IDMSv2, then below field needs to be added to credential_source
  section of credential configuration.
  "imdsv2_session_token_url": "http://169.254.169.254/latest/api/token"

Follow the detailed instructions on how to
`Configure Workload Identity Federation from AWS`_.

.. _Configure Workload Identity Federation from AWS:
    https://cloud.google.com/iam/docs/access-resources-aws

Accessing resources from Microsoft Azure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to access Google Cloud resources from Microsoft Azure, the following
requirements are needed:

- A workload identity pool needs to be created.
- Azure needs to be added as an identity provider in the workload identity pool
  (The Google organization policy needs to allow federation from Azure).
- The Azure tenant needs to be configured for identity federation.
- Permission to impersonate a service account needs to be granted to the
  external identity.
- A credential configuration file needs to be generated. Unlike service account
  credential files, the generated credential configuration file will only
  contain non-sensitive metadata to instruct the library on how to retrieve
  external subject tokens and exchange them for service account access tokens.

Follow the detailed instructions on how to
`Configure Workload Identity Federation from Microsoft Azure`_.

.. _Configure Workload Identity Federation from Microsoft Azure:
    https://cloud.google.com/iam/docs/access-resources-azure

Accessing resources from an OIDC identity provider
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to access Google Cloud resources from an identity provider that
supports `OpenID Connect (OIDC)`_, the following requirements are needed:

- A workload identity pool needs to be created.
- An OIDC identity provider needs to be added in the workload identity pool
  (The Google organization policy needs to allow federation from the identity
  provider).
- Permission to impersonate a service account needs to be granted to the
  external identity.
- A credential configuration file needs to be generated. Unlike service account
  credential files, the generated credential configuration file will only
  contain non-sensitive metadata to instruct the library on how to retrieve
  external subject tokens and exchange them for service account access tokens.

For OIDC providers, the Auth library can retrieve OIDC tokens either from a
local file location (file-sourced credentials) or from a local server
(URL-sourced credentials).

- For file-sourced credentials, a background process needs to be continuously
  refreshing the file location with a new OIDC token prior to expiration.
  For tokens with one hour lifetimes, the token needs to be updated in the file
  every hour. The token can be stored directly as plain text or in JSON format.
- For URL-sourced credentials, a local server needs to host a GET endpoint to
  return the OIDC token. The response can be in plain text or JSON.
  Additional required request headers can also be specified.

Follow the detailed instructions on how to
`Configure Workload Identity Federation from an OIDC identity provider`_.

.. _OpenID Connect (OIDC):
    https://openid.net/connect/
.. _Configure Workload Identity Federation from an OIDC identity provider:
    https://cloud.google.com/iam/docs/access-resources-oidc

Using External Identities
~~~~~~~~~~~~~~~~~~~~~~~~~

External identities (AWS, Azure and OIDC identity providers) can be used with
Application Default Credentials.
In order to use external identities with Application Default Credentials, you
need to generate the JSON credentials configuration file for your external
identity.
Once generated, store the path to this file in the
``GOOGLE_APPLICATION_CREDENTIALS`` environment variable.

.. code-block:: bash

    $ export GOOGLE_APPLICATION_CREDENTIALS=/path/to/config.json

The library can now automatically choose the right type of client and initialize
credentials from the context provided in the configuration file::

    import google.auth

    credentials, project = google.auth.default()

When using external identities with Application Default Credentials,
the ``roles/browser`` role needs to be granted to the service account.
The ``Cloud Resource Manager API`` should also be enabled on the project.
This is needed since :func:`default` will try to auto-discover the project ID
from the current environment using the impersonated credential.
Otherwise, the project ID will resolve to ``None``. You can override the project
detection by setting the ``GOOGLE_CLOUD_PROJECT`` environment variable.

You can also explicitly initialize external account clients using the generated
configuration file.

For Azure and OIDC providers, use :meth:`identity_pool.Credentials.from_info
<google.auth.identity_pool.Credentials.from_info>` or
:meth:`identity_pool.Credentials.from_file
<google.auth.identity_pool.Credentials.from_file>`::

    import json

    from google.auth import identity_pool

    json_config_info = json.loads(function_to_get_json_config())
    credentials = identity_pool.Credentials.from_info(json_config_info)
    scoped_credentials = credentials.with_scopes(
        ['https://www.googleapis.com/auth/cloud-platform'])

For AWS providers, use :meth:`aws.Credentials.from_info
<google.auth.aws.Credentials.from_info>` or
:meth:`aws.Credentials.from_file
<google.auth.aws.Credentials.from_file>`::

    import json

    from google.auth import aws

    json_config_info = json.loads(function_to_get_json_config())
    credentials = aws.Credentials.from_info(json_config_info)
    scoped_credentials = credentials.with_scopes(
        ['https://www.googleapis.com/auth/cloud-platform'])


Impersonated credentials
++++++++++++++++++++++++

Impersonated Credentials allows one set of credentials issued to a user or service account
to impersonate another.  The source credentials must be granted 
the "Service Account Token Creator" IAM role. ::

    from google.auth import impersonated_credentials

    target_scopes = ['https://www.googleapis.com/auth/devstorage.read_only']
    source_credentials = service_account.Credentials.from_service_account_file(
        '/path/to/svc_account.json',
        scopes=target_scopes)

    target_credentials = impersonated_credentials.Credentials(
        source_credentials=source_credentials,
        target_principal='impersonated-account@_project_.iam.gserviceaccount.com',
        target_scopes=target_scopes,
        lifetime=500)
    client = storage.Client(credentials=target_credentials)
    buckets = client.list_buckets(project='your_project')
    for bucket in buckets:
        print(bucket.name)


In the example above `source_credentials` does not have direct access to list buckets
in the target project.  Using `ImpersonatedCredentials` will allow the source_credentials
to assume the identity of a target_principal that does have access.


Downscoped credentials
++++++++++++++++++++++

`Downscoping with Credential Access Boundaries`_ is used to restrict the
Identity and Access Management (IAM) permissions that a short-lived credential
can use.

To downscope permissions of a source credential, a `Credential Access Boundary`
that specifies which resources the new credential can access, as well as
an upper bound on the permissions that are available on each resource, has to
be defined. A downscoped credential can then be instantiated using the
`source_credential` and the `Credential Access Boundary`.

The common pattern of usage is to have a token broker with elevated access
generate these downscoped credentials from higher access source credentials and
pass the downscoped short-lived access tokens to a token consumer via some
secure authenticated channel for limited access to Google Cloud Storage
resources.

.. _Downscoping with Credential Access Boundaries: https://cloud.google.com/iam/docs/downscoping-short-lived-credentials

Token broker ::

    import google.auth

    from google.auth import downscoped
    from google.auth.transport import requests

    # Initialize the credential access boundary rules.
    available_resource = '//storage.googleapis.com/projects/_/buckets/bucket-123'
    available_permissions = ['inRole:roles/storage.objectViewer']
    availability_expression = (
        "resource.name.startsWith('projects/_/buckets/bucket-123/objects/customer-a')"
    )

    availability_condition = downscoped.AvailabilityCondition(
        availability_expression)
    rule = downscoped.AccessBoundaryRule(
        available_resource=available_resource,
        available_permissions=available_permissions,
        availability_condition=availability_condition)
    credential_access_boundary = downscoped.CredentialAccessBoundary(
        rules=[rule])

    # Retrieve the source credentials via ADC. 
    source_credentials, _ = google.auth.default()

    # Create the downscoped credentials.
    downscoped_credentials = downscoped.Credentials(
        source_credentials=source_credentials,
        credential_access_boundary=credential_access_boundary)

    # Refresh the tokens.
    downscoped_credentials.refresh(requests.Request())

    # These values will need to be passed to the Token Consumer.
    access_token = downscoped_credentials.token
    expiry = downscoped_credentials.expiry


For example, a token broker can be set up on a server in a private network.
Various workloads (token consumers) in the same network will send authenticated
requests to that broker for downscoped tokens to access or modify specific google
cloud storage buckets.

The broker will instantiate downscoped credentials instances that can be used to
generate short lived downscoped access tokens that can be passed to the token
consumer. These downscoped access tokens can be injected by the consumer into
`google.oauth2.Credentials` and used to initialize a storage client instance to
access Google Cloud Storage resources with restricted access.

Token Consumer ::

    import google.oauth2

    from google.auth.transport import requests
    from google.cloud import storage

    # Downscoped token retrieved from token broker.
    # The `get_token_from_broker` callable requests a token and an expiry
    # from the token broker.
    downscoped_token, expiry = get_token_from_broker(
        requests.Request(),
        scopes=['https://www.googleapis.com/auth/cloud-platform'])

    # Create the OAuth credentials from the downscoped token and pass a
    # refresh handler to handle token expiration. Passing the original
    # downscoped token or the expiry here is optional, as the refresh_handler
    # will generate the downscoped token on demand.
    credentials = google.oauth2.credentials.Credentials(
        downscoped_token,
        expiry=expiry,
        scopes=['https://www.googleapis.com/auth/cloud-platform'],
        refresh_handler=get_token_from_broker)

    # Initialize a storage client with the oauth2 credentials.
    storage_client = storage.Client(
        project='my_project_id', credentials=credentials)
    # Call GCS APIs.
    # The token broker has readonly access to objects starting with "customer-a"
    # in bucket "bucket-123".
    bucket = storage_client.bucket('bucket-123')
    blob = bucket.blob('customer-a-data.txt')
    print(blob.download_as_bytes().decode("utf-8"))


Another reason to use downscoped credentials is to ensure tokens in flight
always have the least privileges, e.g. Principle of Least Privilege. ::

    # Create the downscoped credentials.
    downscoped_credentials = downscoped.Credentials(
        # source_credentials have elevated access but only a subset of
        # these permissions are needed here.
        source_credentials=source_credentials,
        credential_access_boundary=credential_access_boundary)

    # Pass the token directly.
    storage_client = storage.Client(
        project='my_project_id', credentials=downscoped_credentials)
    # If the source credentials have elevated levels of access, the
    # token in flight here will have limited readonly access to objects
    # starting with "customer-a" in bucket "bucket-123".
    bucket = storage_client.bucket('bucket-123')
    blob = bucket.blob('customer-a-data.txt')
    print(blob.download_as_string())


Note: Only Cloud Storage supports Credential Access Boundaries. Other Google
Cloud services do not support this feature.


Identity Tokens
+++++++++++++++

`Google OpenID Connect`_ tokens are available through :mod:`Service Account <google.oauth2.service_account>`,
:mod:`Impersonated <google.auth.impersonated_credentials>`,
and :mod:`Compute Engine <google.auth.compute_engine>`.  These tokens can be used to
authenticate against `Cloud Functions`_, `Cloud Run`_, a user service behind
`Identity Aware Proxy`_ or any other service capable of verifying a `Google ID Token`_.

ServiceAccount ::

    from google.oauth2 import service_account

    target_audience = 'https://example.com'

    creds = service_account.IDTokenCredentials.from_service_account_file(
            '/path/to/svc.json',
            target_audience=target_audience)


Compute ::

    from google.auth import compute_engine
    import google.auth.transport.requests

    target_audience = 'https://example.com'

    request = google.auth.transport.requests.Request()
    creds = compute_engine.IDTokenCredentials(request,
                            target_audience=target_audience)

Impersonated ::

    from google.auth import impersonated_credentials

    # get target_credentials from a source_credential

    target_audience = 'https://example.com'

    creds = impersonated_credentials.IDTokenCredentials(
                                      target_credentials,
                                      target_audience=target_audience)

If your application runs on `App Engine`_, `Cloud Run`_, `Compute Engine`_, or
has application default credentials set via `GOOGLE_APPLICATION_CREDENTIALS`
environment variable, you can also use `google.oauth2.id_token.fetch_id_token`
to obtain an ID token from your current running environment. The following is an
example ::

    import google.oauth2.id_token
    import google.auth.transport.requests

    request = google.auth.transport.requests.Request()
    target_audience = "https://pubsub.googleapis.com"

    id_token = google.oauth2.id_token.fetch_id_token(request, target_audience)

IDToken verification can be done for various type of IDTokens using the
:class:`google.oauth2.id_token` module. It supports ID token signed with RS256
and ES256 algorithms. However, ES256 algorithm won't be available unless
`cryptography` dependency of version at least 1.4.0 is installed. You can check
the dependency with `pip freeze` or try `from google.auth.crypt import es256`.
The following is an example of verifying ID tokens ::

    from google.auth2 import id_token

    request = google.auth.transport.requests.Request()

    try:
        decoded_token = id_token.verify_token(token_to_verify,request)
    except ValueError:
        # Verification failed.

A sample end-to-end flow using an ID Token against a Cloud Run endpoint maybe ::

    from google.oauth2 import id_token
    from google.oauth2 import service_account
    import google.auth
    import google.auth.transport.requests
    from google.auth.transport.requests import AuthorizedSession

    target_audience = 'https://your-cloud-run-app.a.run.app'
    url = 'https://your-cloud-run-app.a.run.app'

    creds = service_account.IDTokenCredentials.from_service_account_file(
            '/path/to/svc.json', target_audience=target_audience)

    authed_session = AuthorizedSession(creds)

    # make authenticated request and print the response, status_code
    resp = authed_session.get(url)
    print(resp.status_code)
    print(resp.text)

    # to verify an ID Token
    request = google.auth.transport.requests.Request()
    token = creds.token
    print(token)
    print(id_token.verify_token(token,request))

.. _App Engine: https://cloud.google.com/appengine/
.. _Cloud Functions: https://cloud.google.com/functions/
.. _Cloud Run: https://cloud.google.com/run/
.. _Identity Aware Proxy: https://cloud.google.com/iap/
.. _Google OpenID Connect: https://developers.google.com/identity/protocols/OpenIDConnect
.. _Google ID Token: https://developers.google.com/identity/protocols/OpenIDConnect#validatinganidtoken

Making authenticated requests
-----------------------------

Once you have credentials you can attach them to a *transport*. You can then
use this transport to make authenticated requests to APIs. google-auth supports
several different transports. Typically, it's up to your application or an
opinionated client library to decide which transport to use.

Requests
++++++++

The recommended HTTP transport is :mod:`google.auth.transport.requests` which
uses the `Requests`_ library. To make authenticated requests using Requests
you use a custom `Session`_ object::

    from google.auth.transport.requests import AuthorizedSession

    authed_session = AuthorizedSession(credentials)

    response = authed_session.get(
        'https://www.googleapis.com/storage/v1/b')

.. _Requests: http://docs.python-requests.org/en/master/
.. _Session: http://docs.python-requests.org/en/master/user/advanced/#session-objects

urllib3
+++++++

:mod:`urllib3` is the underlying HTTP library used by Requests and can also be
used with google-auth. urllib3's interface isn't as high-level as Requests but
it can be useful in situations where you need more control over how HTTP
requests are made. To make authenticated requests using urllib3 create an
instance of :class:`google.auth.transport.urllib3.AuthorizedHttp`::

    from google.auth.transport.urllib3 import AuthorizedHttp

    authed_http = AuthorizedHttp(credentials)

    response = authed_http.request(
        'GET', 'https://www.googleapis.com/storage/v1/b')

You can also construct your own :class:`urllib3.PoolManager` instance and pass
it to :class:`~google.auth.transport.urllib3.AuthorizedHttp`::

    import urllib3

    http = urllib3.PoolManager()
    authed_http = AuthorizedHttp(credentials, http)

gRPC
++++

`gRPC`_ is an RPC framework that uses `Protocol Buffers`_ over `HTTP 2.0`_.
google-auth can provide `Call Credentials`_ for gRPC. The easiest way to do
this is to use google-auth to create the gRPC channel::

    import google.auth.transport.grpc
    import google.auth.transport.requests

    http_request = google.auth.transport.requests.Request()

    channel = google.auth.transport.grpc.secure_authorized_channel(
        credentials, http_request, 'pubsub.googleapis.com:443')

.. note:: Even though gRPC is its own transport, you still need to use one of
    the other HTTP transports with gRPC. The reason is that most credential
    types need to make HTTP requests in order to refresh their access token.
    The sample above uses the Requests transport, but any HTTP transport can
    be used. Additionally, if you know that your credentials do not need to
    make HTTP requests in order to refresh (as is the case with
    :class:`jwt.Credentials`) then you can specify ``None``.

Alternatively, you can create the channel yourself and use
:class:`google.auth.transport.grpc.AuthMetadataPlugin`::

    import grpc

    metadata_plugin = AuthMetadataPlugin(credentials, http_request)

    # Create a set of grpc.CallCredentials using the metadata plugin.
    google_auth_credentials = grpc.metadata_call_credentials(
        metadata_plugin)

    # Create SSL channel credentials.
    ssl_credentials = grpc.ssl_channel_credentials()

    # Combine the ssl credentials and the authorization credentials.
    composite_credentials = grpc.composite_channel_credentials(
        ssl_credentials, google_auth_credentials)

    channel = grpc.secure_channel(
        'pubsub.googleapis.com:443', composite_credentials)

You can use this channel to make a gRPC stub that makes authenticated requests
to a gRPC service::

    from google.pubsub.v1 import pubsub_pb2

    pubsub = pubsub_pb2.PublisherStub(channel)

    response = pubsub.ListTopics(
        pubsub_pb2.ListTopicsRequest(project='your-project'))


.. _gRPC: http://www.grpc.io/
.. _Protocol Buffers:
    https://developers.google.com/protocol-buffers/docs/overview
.. _HTTP 2.0:
    http://www.grpc.io/docs/guides/wire.html
.. _Call Credentials:
    http://www.grpc.io/docs/guides/auth.html
