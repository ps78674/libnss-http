**NSS module for rest/http.**

**Configureation**\
/etc/nsswitch.conf:
```
passwd:         compat http
group:          compat http
shadow:         compat http
```
/etc/nss_http_.conf:
```
HTTPSERVER=https://localhost/api/nss
TIMEOUT=3
#DEBUG=true
```

**Endpoints**\
/passwd
```
[
    {
        "pw_name": "iivanov",
        "pw_passwd": "x",
        "pw_uid": 1111,
        "pw_gid": 2222,
        "pw_gecos": "Ivan Ivanov",
        "pw_dir": "/home/iivanov",
        "pw_shell": "/bin/bash"
    }
]
```
/shadow
```
[
    {
        "sp_namp": "iivanov",
        "sp_pwdp": "0000000000000000000000000000000000000000000000000000000",
        "sp_lstchg": 12345,
        "sp_min": 0,
        "sp_max": 99999,
        "sp_warn": 7,
        "sp_inact": null,
        "sp_expire": null,
        "sp_flag": null
    }
]
```
/group
```
[
    {
        "gr_name": "admins",
        "gr_passwd": "x",
        "gr_gid": 3333,
        "gr_mem": [
            "iivanov"
        ]
    }
]
```