## **NSS module for rest/http auth.**
### **Configuration**
/etc/nsswitch.conf:
```
passwd:         compat http
group:          compat http
shadow:         compat http
```
/etc/nss_http.conf:
```
HTTPSERVER=https://<API_SERVER>/api/nss
TIMEOUT=3
#DEBUG=true
```

### **Endpoints**
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
The passwd endpoint should expect to receive 1 of 2 mutually exclusive query parameters. name, containing a username or uid containing a user id. If either of these query parameters are received, a single object should be returned for the requested username/uid.

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
The shadow endpoint should expect to receive a query parameter of name, containing a username. If this query parameter is received, a single object should be returned for the requested username.

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
The group endpoint should expect to receive 1 of 2 mutually exclusive query parameters. name, containing a username or gid containing a group id. If either of these query parameters are received, a single object should be returned for the requested groupname/gid.

### **Sources**
https://github.com/gmjosack/nss_http  
https://github.com/json-c/json-c  
https://curl.haxx.se/libcurl/c/  
