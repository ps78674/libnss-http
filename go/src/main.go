package main

// #include <nss.h>
import "C"
import (
	"bufio"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

const (
	nssStatusTryagain = C.NSS_STATUS_TRYAGAIN
	// nssStatusUnavail  = C.NSS_STATUS_UNAVAIL
	nssStatusNotfound = C.NSS_STATUS_NOTFOUND
	nssStatusSuccess  = C.NSS_STATUS_SUCCESS
)

type nssStatus int32

type passwd struct {
	Username string `json:"pw_name"`
	Password string `json:"pw_passwd"`
	UID      uint   `json:"pw_uid"`
	GID      uint   `json:"pw_gid"`
	Gecos    string `json:"pw_gecos"`
	Dir      string `json:"pw_dir"`
	Shell    string `json:"pw_shell"`
}

type group struct {
	Groupname string   `json:"gr_name"`
	Password  string   `json:"gr_passwd"`
	GID       uint     `json:"gr_gid"`
	Members   []string `json:"gr_mem"`
}

type shadow struct {
	Username        string      `json:"sp_namp"`
	Password        string      `json:"sp_pwdp"`
	LastChange      int         `json:"sp_lstchg"`
	MinChange       int         `json:"sp_min"`
	MaxChange       int         `json:"sp_max"`
	PasswordWarn    int         `json:"sp_warn"`
	InactiveLockout interface{} `json:"sp_inact"`
	ExpirationDate  interface{} `json:"sp_expire"`
	Reserved        interface{} `json:"sp_flag"`
}

type response struct {
	Type string
	Data []byte
	Err  error
}

var (
	passwdEntries    []passwd
	shadowEntries    []shadow
	groupEntries     []group
	passwdEntryIndex int
	shadowEntryIndex int
	groupEntryIndex  int
)

type config struct {
	url     string
	debug   bool
	timeout int
}

var (
	configFile = "/etc/nss_http.conf"

	hostname string
	conf     config
)

func debugPrint(s string) {
	if conf.debug {
		os.Stderr.WriteString(s)
	}
}

func readConfig() {
	f, err := os.Open(configFile)
	if err != nil {
		msg := fmt.Sprintf("error reading config %s: %s\n", configFile, err)
		os.Stderr.WriteString(msg)
		os.Exit(1)
	}

	defer f.Close()

	scanner := bufio.NewScanner(f)
	for scanner.Scan() {
		confline := strings.SplitN(scanner.Text(), "=", 2)
		switch strings.TrimSpace(confline[0]) {
		case "HTTPSERVER", "APIURL":
			conf.url = strings.TrimSpace(confline[1])
		case "DEBUG":
			conf.debug, _ = strconv.ParseBool(strings.TrimSpace(confline[1]))
		case "TIMEOUT", "HTTPTIMEOUT":
			conf.timeout, _ = strconv.Atoi(strings.TrimSpace(confline[1]))
		}
	}
}

// get fqdn - not using uname
func getHostname() (string, error) {
	// DEBUG
	debugPrint("getHostname\n")

	hostname, err := os.Hostname()
	if err != nil {
		return "", err
	}

	addrs, err := net.LookupIP(hostname)
	if err != nil {
		return hostname, nil
	}

	for _, addr := range addrs {
		if ipv4 := addr.To4(); ipv4 != nil {
			ip, err := ipv4.MarshalText()
			if err != nil {
				return hostname, nil
			}
			hosts, err := net.LookupAddr(string(ip))
			if err != nil || len(hosts) == 0 {
				return hostname, nil
			}
			fqdn := hosts[0]
			return strings.TrimSuffix(fqdn, "."), nil // return fqdn without trailing dot
		}
	}
	return hostname, nil
}

func doRequest(reqType string, hostname string, ch chan<- response) {
	// DEBUG
	debugPrint("doRequest\n")

	reqURL := fmt.Sprintf("%s/%s?format=json&hostname=%s", conf.url, reqType, hostname)

	req, err := http.NewRequest("GET", reqURL, nil)
	if err != nil {
		err := fmt.Errorf("error creating request: %s", err)
		ch <- response{Err: err}
		return
	}

	req.Header.Add("User-Agent", "NSS-goHTTP")

	var transport = &http.Transport{
		Dial: (&net.Dialer{
			Timeout: time.Second * time.Duration(conf.timeout),
		}).Dial,
		TLSHandshakeTimeout: time.Second * time.Duration(conf.timeout),
	}

	client := &http.Client{
		Timeout:   time.Second * time.Duration(conf.timeout),
		Transport: transport,
	}

	resp, err := client.Do(req)
	if err != nil {
		err := fmt.Errorf("error getting response: %s", err)
		ch <- response{Err: err}
		return
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		err := fmt.Errorf("error reading response data: %s", err)
		ch <- response{Err: err}
		return
	}

	ch <- response{Type: reqType, Data: body}
}

func init() {
	readConfig()

	hostname, err := getHostname()
	if err != nil {
		msg := fmt.Sprintf("error getting system hostname: %s\n", err)
		os.Stderr.WriteString(msg)
		os.Exit(1)
	}

	ch := make(chan response)
	defer close(ch)

	for _, reqType := range []string{"passwd", "shadow", "group"} {
		go doRequest(reqType, hostname, ch)
	}

	for i := 0; i < 3; i++ {
		resp := <-ch

		if resp.Err != nil {
			os.Stderr.WriteString(resp.Err.Error() + "\n")
			continue
		}

		switch resp.Type {
		case "passwd":
			if e := json.Unmarshal(resp.Data, &passwdEntries); e != nil {
				msg := fmt.Sprintf("error unmarshalling passwd data: %s\n", e)
				os.Stderr.WriteString(msg)
			}
		case "shadow":
			if e := json.Unmarshal(resp.Data, &shadowEntries); e != nil {
				msg := fmt.Sprintf("error unmarshalling shadow data: %s\n", e)
				os.Stderr.WriteString(msg)
			}
		case "group":
			if e := json.Unmarshal(resp.Data, &groupEntries); e != nil {
				msg := fmt.Sprintf("error unmarshalling group data: %s\n", e)
				os.Stderr.WriteString(msg)
			}
		}
	}
}

func main() {}
