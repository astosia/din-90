// Clay Config: see https://github.com/pebble/clay
module.exports = [
  {
    "type": "heading",
    "defaultValue": "Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Theme settings"
      },   
      {
        "type": "color",
        "messageKey": "Back1Color",
        "defaultValue": "0x000000",
        "label": "Background"
      },
      {
        "type": "color",
        "messageKey": "HourColor",
        "defaultValue": "0x00FFFF",
        "label": "Hour Text"
      },
      {
        "type": "color",
        "messageKey": "MinColor",
        "defaultValue": "0xFFFFAA",
        "label": "Minute Text"
      }, 
      {
        "type": "color",
        "messageKey": "Text1Color",
        "defaultValue": "0xFFFFFF",
        "label": "Weather Text"
      }, 
      {
        "type": "color",
        "messageKey": "Text2Color",
        "defaultValue": "0xFFFFFF",
        "label": "Complications Left"
      }, 
       {
        "type": "color",
        "messageKey": "Text3Color",
        "defaultValue": "0xFFFFFF",
        "label": "Complications Right"
      }, 
      {"type": "section",
       "items": [
         {
           "type": "heading",
           "defaultValue": "Night Theme",
           "size":4
         } ,
         {
           "type": "toggle",
           "messageKey": "NightTheme",
           "label": "Activate Night Theme",
           "defaultValue": false,        
         },
         {
           "type": "color",
           "messageKey": "Back1ColorN",
           "defaultValue": "0xFFFFFF",
           "label": "Background"
         },
         {
        "type": "color",
        "messageKey": "HourColorN",
        "defaultValue": "0x000000",
        "label": "Hour Text"
      },
      {
        "type": "color",
        "messageKey": "MinColorN",
        "defaultValue": "0x000000",
        "label": "Minute Text"
      }, 
         {
           "type": "color",
           "messageKey": "Text1ColorN",
           "defaultValue": "0x000000",
           "label": "Weather Text"
         }, 
       
         {
           "type": "color",
           "messageKey": "Text2ColorN",
           "defaultValue": "0x000000",
           "label": "Complications Left"
         },
          {
        "type": "color",
        "messageKey": "Text3ColorN",
        "defaultValue": "0xFFFFFF",
        "label": "Complications Right"
      }, 
       ]
         }
       ]
      },
      {
        "type": "section",
        "items": [
          {
            "type": "heading",
            "defaultValue": "Connection settings"
          },     
          {
            "type": "toggle",
            "messageKey": "WeatherUnit",
            "label": "Temperature in Fahrenheit",
            "defaultValue": false,
          },
          {
            "type": "select",
            "messageKey": "WeatherProv",
            "defaultValue": "wu",
            "label": "Weather Provider",
            "options": [
              {
                "label": "OpenWeatherMap",
                "value": "owm"
              },
              {
                "label": "WeatherUnderground",
                "value": "wu"
              }
            ]
          },
          {
            "type": "input",
            "messageKey": "APIKEY_User",
            "defaultValue": "",
            "label": "API Key",
            "description": "Paste your API Key here. If left blank, the watch will attempt to request your pmkey.xyz",
            "attributes": {
              "placeholder": "eg: xxxxxxxxxxxxx"
            }
          },
          {
            "type": "input",
            "messageKey": "EmailPMKEY",
            "defaultValue": "",
            "label": "pmkey.xyz User",
            "description": "pmkey.xyz is a free service for Pebble users that allows you to safely store all your API keys in a single place. Check <a href=https://www.pmkey.xyz/>pmkey.xyz</a> ",
            "attributes": {
              "placeholder": "eg: jane.smith@pmkey.xyz",
              "type": "email"
            }
          },
          {
            "type": "input",
            "messageKey": "PINPMKEY",
            "defaultValue": "",
            "label": "pmkey.xyz PIN",
            "attributes": {
              "placeholder": "eg: 12345"
            }
          },
          {
            "type": "slider",
            "messageKey": "UpSlider",
            "defaultValue": 30,
            "label": "Update frequency (minutes)",
            "description": "More frequent requests will drain your phone battery faster",
            "min": 15,
            "max": 120,
            "step": 15},
        ]
          },
          {
          "type": "submit",
          "defaultValue":"SAVE"
          },
          {
          "type": "heading",
          "defaultValue": "version v1.0",
          "size":6
          },
          {
          "type": "heading",
          "defaultValue": "Made in UK",
          "size":6
          }
        ];