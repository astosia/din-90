//function () { "use strict";

// shortcuts for easier to read formulas

var PI   = Math.PI,
    sin  = Math.sin,
    cos  = Math.cos,
    tan  = Math.tan,
    asin = Math.asin,
    atan = Math.atan2,
    acos = Math.acos,
    rad  = PI / 180;

// sun calculations are based on http://aa.quae.nl/en/reken/zonpositie.html formulas


// date/time constants and conversions

var dayMs = 1000 * 60 * 60 * 24,
    J1970 = 2440588,
    J2000 = 2451545;

function toJulian(date) {
    return date.valueOf() / dayMs - 0.5 + J1970;
}
function fromJulian(j) {
    return new Date((j + 0.5 - J1970) * dayMs);
}
function toDays(date) {
    return toJulian(date) - J2000;
}


// general calculations for position

var e = rad * 23.4397; // obliquity of the Earth

function getRightAscension(l, b) {
    return atan(sin(l) * cos(e) - tan(b) * sin(e), cos(l));
}
function getDeclination(l, b) {
    return asin(sin(b) * cos(e) + cos(b) * sin(e) * sin(l));
}
function getAzimuth(H, phi, dec) {
    return atan(sin(H), cos(H) * sin(phi) - tan(dec) * cos(phi));
}
function getAltitude(H, phi, dec) {
    return asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));
}
function getSiderealTime(d, lw) {
    return rad * (280.16 + 360.9856235 * d) - lw;
}


// general sun calculations

function getSolarMeanAnomaly(d) {
    return rad * (357.5291 + 0.98560028 * d);
}
function getEquationOfCenter(M) {
    return rad * (1.9148 * sin(M) + 0.02 * sin(2 * M) + 0.0003 * sin(3 * M));
}
function getEclipticLongitude(M, C) {
    var P = rad * 102.9372; // perihelion of the Earth
    return M + C + P + PI;
}
function getSunCoords(d) {

    var M = getSolarMeanAnomaly(d),
        C = getEquationOfCenter(M),
        L = getEclipticLongitude(M, C);

    return {
        dec: getDeclination(L, 0),
        ra: getRightAscension(L, 0)
    };
}


var SunCalc = {};


// calculates sun position for a given date and latitude/longitude

SunCalc.getPosition = function (date, lat, lng) {

    var lw  = rad * -lng,
        phi = rad * lat,
        d   = toDays(date),

        c  = getSunCoords(d),
        H  = getSiderealTime(d, lw) - c.ra;

    return {
        azimuth: getAzimuth(H, phi, c.dec),
        altitude: getAltitude(H, phi, c.dec)
    };
};


// sun times configuration (angle, morning name, evening name)

var times = [
    [-0.83, 'sunrise',       'sunset'      ],
    [ -0.3, 'sunriseEnd',    'sunsetStart' ],
    [   -6, 'dawn',          'dusk'        ],
    [  -12, 'nauticalDawn',  'nauticalDusk'],
    [  -18, 'nightEnd',      'night'       ],
    [    6, 'goldenHourEnd', 'goldenHour'  ]
];

// adds a custom time to the times config

SunCalc.addTime = function (angle, riseName, setName) {
    times.push([angle, riseName, setName]);
};


// calculations for sun times

var J0 = 0.0009;

function getJulianCycle(d, lw) {
    return Math.round(d - J0 - lw / (2 * PI));
}
function getApproxTransit(Ht, lw, n) {
    return J0 + (Ht + lw) / (2 * PI) + n;
}
function getSolarTransitJ(ds, M, L) {
    return J2000 + ds + 0.0053 * sin(M) - 0.0069 * sin(2 * L);
}
function getHourAngle(h, phi, d) {
    return acos((sin(h) - sin(phi) * sin(d)) / (cos(phi) * cos(d)));
}


// calculates sun times for a given date and latitude/longitude

SunCalc.getTimes = function (date, lat, lng) {

    var lw  = rad * -lng,
        phi = rad * lat,
        d   = toDays(date),

        n  = getJulianCycle(d, lw),
        ds = getApproxTransit(0, lw, n),

        M = getSolarMeanAnomaly(ds),
        C = getEquationOfCenter(M),
        L = getEclipticLongitude(M, C),

        dec = getDeclination(L, 0),

        Jnoon = getSolarTransitJ(ds, M, L);


    // returns set time for the given sun altitude
    function getSetJ(h) {
        var w = getHourAngle(h, phi, dec),
            a = getApproxTransit(w, lw, n);

        return getSolarTransitJ(a, M, L);
    }


    var result = {
        solarNoon: fromJulian(Jnoon),
        nadir: fromJulian(Jnoon - 0.5)
    };

    var i, len, time, angle, morningName, eveningName, Jset, Jrise;

    for (i = 0, len = times.length; i < len; i += 1) {
        time = times[i];

        Jset = getSetJ(time[0] * rad);
        Jrise = Jnoon - (Jset - Jnoon);

        result[time[1]] = fromJulian(Jrise);
        result[time[2]] = fromJulian(Jset);
    }

    return result;
};


// moon calculations, based on http://aa.quae.nl/en/reken/hemelpositie.html formulas

function getMoonCoords(d) { // geocentric ecliptic coordinates of the moon

    var L = rad * (218.316 + 13.176396 * d), // ecliptic longitude
        M = rad * (134.963 + 13.064993 * d), // mean anomaly
        F = rad * (93.272 + 13.229350 * d),  // mean distance

        l  = L + rad * 6.289 * sin(M), // longitude
        b  = rad * 5.128 * sin(F),     // latitude
        dt = 385001 - 20905 * cos(M);  // distance to the moon in km

    return {
        ra: getRightAscension(l, b),
        dec: getDeclination(l, b),
        dist: dt
    };
}

SunCalc.getMoonPosition = function (date, lat, lng) {

    var lw  = rad * -lng,
        phi = rad * lat,
        d   = toDays(date),

        c = getMoonCoords(d),
        H = getSiderealTime(d, lw) - c.ra,
        h = getAltitude(H, phi, c.dec);

    // altitude correction for refraction
    h = h + rad * 0.017 / tan(h + rad * 10.26 / (h + rad * 5.10));

    return {
        azimuth: getAzimuth(H, phi, c.dec),
        altitude: h,
        distance: c.dist
    };
};


// calculations for illuminated fraction of the moon,
// based on http://idlastro.gsfc.nasa.gov/ftp/pro/astro/mphase.pro formulas

SunCalc.getMoonFraction = function (date) {

    var d = toDays(date),
        s = getSunCoords(d),
        m = getMoonCoords(d),

        sdist = 149598000, // distance from Earth to Sun in km

        phi = acos(sin(s.dec) * sin(m.dec) + cos(s.dec) * cos(m.dec) * cos(s.ra - m.ra)),
        inc = atan(sdist * sin(phi), m.dist - sdist * cos(phi));

    return (1 + cos(inc)) / 2;
};

var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};
// Request for WU
function locationSuccessWU(pos){
  //Request WU
  var lat=pos.coords.latitude;
  var lon= pos.coords.longitude;
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
        var d = new Date();
        var sunTimes = SunCalc.getTimes(d, lat, lon);
        var sunsetStrhr = ('0'+sunTimes.sunset.getHours()).substr(-2);
        var sunsetStrmin = ('0'+sunTimes.sunset.getMinutes()).substr(-2);
        var sunsetStr = String(sunsetStrhr + ":" + sunsetStrmin);   
  var units = unitsToString(settings.WeatherUnit);
  var keyAPIwu = localStorage.getItem('wuKey');
  var userKeyApi=settings.APIKEY_User;
  var endapikey=apikeytouse(userKeyApi,keyAPIwu);
  var langtouse=translatewu(navigator.language);
  // Construct URL
  var urlWU = "http://api.wunderground.com/api/"+
      endapikey + "/conditions_v11/astronomy_v11/lang:"+langtouse+"/q/"+
      lat+","+lon+
      ".json";
  console.log("WUUrl= " + urlWU);
  xhrRequest(encodeURI(urlWU), 'GET', function(responseText) {
    // responseText contains a JSON object with weather info
    var json = JSON.parse(responseText);
    localStorage.setItem("OKAPI", 0);
    // Temperature
    var tempf = Math.round(json.current_observation.temp_f)+'\xB0F';// units;
    var tempc = Math.round(json.current_observation.temp_c)+ '\xB0C';
    var tempwu=temptousewu(units,tempf,tempc);
    // Condition
    var condwu=json.current_observation.weather;
    var condclean=replaceDiacritics(condwu);
    // City
    var citywu = json.current_observation.display_location.city;
    var cityclean=replaceDiacritics(citywu);
    // Sunrise and Sunset
    var sunrisewu=parseInt(json.sun_phase.sunrise.hour*100)+parseInt(json.sun_phase.sunrise.minute*1);
    var sunsetwu=parseInt(json.sun_phase.sunset.hour*100)+parseInt(json.sun_phase.sunset.minute*1);
    var sunsetStrhr = parseInt(json.sun_phase.sunset.hour*1);
        var sunsetStrmin = ('0' + parseInt(json.sun_phase.sunset.minute*1)).substr(-2);
    var sunsetStr2 = String(sunsetStrhr + ':' + sunsetStrmin);   
var windkts = Math.round(json.current_observation.wind_mph *0.869);
 //       var winddir = json.current_observation.wind_dir;
    var wind = String(windkts + "kts");
    localStorage.setItem("OKAPI", 1);
    console.log("OK API");
    // Assemble dictionary
    var dictionary = {
      "WeatherTemp": tempwu,
      "WeatherCond": condclean,
      "HourSunset": sunsetwu,
      "HourSunrise":sunrisewu,
      "WeatherWind" : wind,
      "WEATHER_SUNSET_KEY":sunsetStr,
      "NameLocation": cityclean
    };
    // Send to Pebble
    Pebble.sendAppMessage(dictionary,function(e) {console.log("Weather from WU sent to Pebble successfully!");},
                                     function(e) {console.log("Error sending WU info to Pebble!");}
                                    );
  });
}


// Request for OWM
function locationSuccessOWM(pos){
  //Request OWM
  var lat=pos.coords.latitude;
  var lon= pos.coords.longitude;
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
        var d = new Date();
        var sunTimes = SunCalc.getTimes(d, lat, lon);
        var sunsetStrhr = ('0'+sunTimes.sunset.getHours()).substr(-2);
        var sunsetStrmin = ('0'+sunTimes.sunset.getMinutes()).substr(-2);
        var sunsetStr = String(sunsetStrhr + ":" + sunsetStrmin);   
  var keyAPIowm=localStorage.getItem('owmKey');
  var userKeyApi=settings.APIKEY_User;
  var endapikey=apikeytouse(userKeyApi,keyAPIowm);  
  var units = unitsToString(settings.WeatherUnit);
  var unitsOWM=unitsToStringOWM(settings.WeatherUnit);
  var langtouse=translate(navigator.language);
  // Construct URL
  var urlOWM = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      lat + "&lon=" + lon +
      '&appid=' + endapikey+
      '&units='+unitsOWM+
      '&lang='+langtouse;
  console.log("OWMUrl= " + urlOWM);
  // Send request to OpenWeatherMap
  xhrRequest(encodeURI(urlOWM), 'GET',function(responseText) {
    // responseText contains a JSON object with weather info
    var json = JSON.parse(responseText);
    localStorage.setItem("OKAPI", 0);
    // Temperature
    var tempowm = Math.round(json.main.temp)+'\xB0'+units;
    // Conditions
    var condowm=json.weather[0].main;//description;
    var condclean=replaceDiacritics(condowm);
    // Sunrise and Sunset
    var auxsunowm =new Date(json.sys.sunrise*1000);
    var sunriseowm=auxsunowm.getHours()*100+auxsunowm.getMinutes();
    var auxsetowm =new Date(json.sys.sunset*1000);
    var sunsetowm=auxsetowm.getHours()*100+auxsetowm.getMinutes();
    // Location
    var cityowm=json.name;
    var cityclean=replaceDiacritics(cityowm);
    var sunsetStrhr = ('0'+auxsetowm.getHours()).substr(-2);
        var sunsetStrmin = ('0'+auxsetowm.getMinutes()).substr(-2);
        var sunsetStr2 = String(sunsetStrhr + ':' + sunsetStrmin); 
 var windkts = Math.round(json.wind.speed * 1.94384);
 //       var winddir = json.current_observation.wind_dir;
    var wind = String(windkts + "kts");  
    localStorage.setItem("OKAPI", 1);
    console.log("OK API");
    // Assemble dictionary using our keys
    var dictionary = {
      "WeatherTemp": tempowm,
      "WeatherCond": condclean,
      "HourSunset": sunsetowm,
      "HourSunrise":sunriseowm,
      "WeatherWind" : wind,
          "WEATHER_SUNSET_KEY":sunsetStr,
      "NameLocation": cityclean
    };
    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
                          function(e) {console.log("Weather from OWM sent to Pebble successfully!");},
                          function(e) { console.log("Error sending OWM info to Pebble!");}
                         );
  });
}
function locationError(err) {
  console.log("Error requesting geolocation!");
  //Send response null
  var location="";
  // Assemble dictionary using our keys
  var dictionary = {
    "NameLocation": location};
  Pebble.sendAppMessage(dictionary,
                        function(e) {
                          console.log("Null key sent to Pebble successfully!");
                        },
                        function(e) {
                          console.log("Null key error sending to Pebble!");
                        }
                       );
}
function getinfo() {
  // Get keys from pmkey
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var email=settings.EmailPMKEY;
  var pin=settings.PINPMKEY;
  if (email !== undefined && pin !== undefined) {
    //Request API from pmkey.xyz
    var urlpmk='https://pmkey.xyz/search/?email='+email+"&pin="+pin;
    console.log("Url PMKEY is "+ urlpmk);
    var keys = parseInt(localStorage.getItem("OKAPI"));
    console.log("Flag keys is " + keys);
    if (keys===0){
      xhrRequest(encodeURI(urlpmk),'GET',
                 function(responseText){
                   var jsonpmk=JSON.parse(responseText);
                   var wuKey=jsonpmk.keys.weather.wu;
                   var owmKey=jsonpmk.keys.weather.owm;
                   localStorage.setItem("wuKey", wuKey);
                   localStorage.setItem("owmKey", owmKey);
                 }
                );
    }
  }  
  var weatherprov=settings.WeatherProv;
  if (weatherprov=="wu"){
    console.log("Ready from WU");
    navigator.geolocation.getCurrentPosition(
      locationSuccessWU,
      locationError,
      {enableHighAccuracy:true,timeout: 15000, maximumAge: 1000}
    );
  }
  else {
    console.log("Ready from OWM");
    navigator.geolocation.getCurrentPosition(
      locationSuccessOWM,
      locationError,
      {enableHighAccuracy:true,timeout: 15000, maximumAge: 1000}
    );
  }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
                        function(e) {
                          console.log("Starting Watchface!");
                          localStorage.setItem("OKAPI", 0);
                          // Get the initial weather
                          getinfo();
                        }
                       );
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
                        function(e) {
                          console.log("Requesting geoposition!");
                          getinfo();
                        }
                       );
// Listen for when the Config app changes
Pebble.addEventListener('webviewclosed',
                        function(e) {
                          console.log("Updating config!");
                          getinfo();
                        }
                       );


//functions and mappings
function unitsToStringOWM(unit) {
  if (unit) {
    return 'imperial';
  }
  return 'metric';
}
function unitsToString(unit) {
  if (unit) {
    return 'F';
  }
  return 'C';
}
function translate(langloc){
  if (langloc==='es-ES'){
    return 'es';
  }
  else if (langloc==='fr_FR'){
    return 'fr';
  }
  else if (langloc==='de_DE'){
    return 'de';
  }
  else if (langloc==='it_IT'){
    return 'it';
  }
  else if (langloc==='pt_PT'){
    return 'pt';
  }
  else {
    return 'en';
  }
}
function translatewu(langloc){
  if (langloc==='es-ES'){
    return 'SP';
  }
  else if (langloc==='fr_FR'){
    return 'FR';
  }
  else if (langloc==='de_DE'){
    return 'DL';
  }
  else if (langloc==='it_IT'){
    return 'IT';
  }
  else if (langloc==='pt_PT'){
    return 'BR';
  }
  else {
    return 'EN';
  }
}
function temptousewu(unit,tempf,tempc){
  if (unit=="F"){
    return tempf; }
  else return tempc;
}
function replaceDiacritics(s){
    var diacritics =[
        /[\300-\306]/g, /[\340-\346]/g,  // A, a
        /[\310-\313]/g, /[\350-\353]/g,  // E, e
        /[\314-\317]/g, /[\354-\357]/g,  // I, i
        /[\322-\330]/g, /[\362-\370]/g,  // O, o
        /[\331-\334]/g, /[\371-\374]/g,  // U, u
        /[\321]/g, /[\361]/g, // N, n
        /[\307]/g, /[\347]/g, // C, c
    ];

    var chars = ['A','a','E','e','I','i','O','o','U','u','N','n','C','c'];

    for (var i = 0; i < diacritics.length; i++)
    {
        s = s.replace(diacritics[i],chars[i]);
    }
  var end=s;
  return end;
}

function apikeytouse(APIUser,APIPMKEY){
  if (APIUser===""){
    console.log("Using pmkey");
    return APIPMKEY;
  }
  else {
    console.log("Using Key User");
    return APIUser;
  }
}