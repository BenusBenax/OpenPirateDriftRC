#include "WebConfigurator.h"

#if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
#include "AuxChannelOutputs.h"
#endif

WebConfigurator::WebConfigurator()
:
server(80)
{

}



void WebConfigurator::begin(
    Settings& settingsRef,
    GyroController& gyroRef,
    RadioInput& steeringRadioRef,
    RadioInput& gainRadioRef,
    RadioInput& throttleRadioRef,
    BlackboxLogger& blackboxRef,
    IMU* imuRef,
    bool imuReadyRef,
    CrsfInput* crsfRef,
    ServoOutput* servoRef,
    EscOutput* escRef,
    volatile bool* hwTestFlagRef,
    uint8_t motorOutputPinRef
)
{
    settings =
        &settingsRef;

    gyro =
        &gyroRef;

    steeringRadio =
        &steeringRadioRef;

    gainRadio =
        &gainRadioRef;

    throttleRadio =
        &throttleRadioRef;

    blackbox =
        &blackboxRef;

    imu =
        imuRef;

    imuOk =
        imuReadyRef;

    crsf =
        crsfRef;

    servoOut =
        servoRef;

    escOut =
        escRef;

    hardwareTestFlag =
        hwTestFlagRef;

    motorOutputPin =
        motorOutputPinRef;

    server.on(
        "/",
        HTTP_GET,
        [this]()
        {
            handleRoot();
        }
    );

    server.on(
        "/save",
        HTTP_POST,
        [this]()
        {
            handleSave();
        }
    );

    server.on(
        "/live-status",
        HTTP_GET,
        [this]()
        {
            handleLiveStatus();
        }
    );

    server.on(
        "/create-profile",
        HTTP_POST,
        [this]()
        {
            handleProfileCreate();
        }
    );

    server.on(
        "/activate-profile",
        HTTP_POST,
        [this]()
        {
            handleProfileActivate();
        }
    );

    server.on(
        "/delete-profile",
        HTTP_POST,
        [this]()
        {
            handleProfileDelete();
        }
    );

    server.on(
        "/blackbox.csv",
        HTTP_GET,
        [this]()
        {
            handleLogDownload();
        }
    );

    server.on(
        "/clear-log",
        HTTP_POST,
        [this]()
        {
            handleLogClear();
        }
    );

    server.on(
        "/api/test_servo",
        HTTP_POST,
        [this]()
        {
            handleTestServo();
        }
    );

    server.on(
        "/api/test_motor",
        HTTP_POST,
        [this]()
        {
            handleTestMotor();
        }
    );

    server.onNotFound(
        [this]()
        {
            handleNotFound();
        }
    );

    server.begin();

    running = true;

    Serial.println(
        "Web configurator started"
    );
}



void WebConfigurator::update()
{
    if(!running)
    {
        return;
    }

    server.handleClient();
}



bool WebConfigurator::isRunning()
{
    return running;
}



void WebConfigurator::handleRoot()
{
    if(settings == nullptr)
    {
        server.send(
            503,
            "text/plain",
            "Settings unavailable"
        );

        return;
    }

    String html;

    html.reserve(20000);

    html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<title>PirateDriftRC Config</title><style>");
    html += F("body{font-family:system-ui,Arial,sans-serif;margin:0;background:#0d1117;color:#e8e3d9}");
    html += F("main{max-width:760px;margin:0 auto;padding:18px}");
    html += F("h1{font-size:28px;margin:8px 0 2px}h2{font-size:18px;margin:22px 0 10px}");
    html += F(".sub{color:#8b949e;margin-bottom:20px}.card{border:1px solid #30363d;border-radius:4px;padding:14px;margin:12px 0;background:#161b22}");
    html += F("label{display:block;font-size:13px;color:#e8e3d9;margin:12px 0 5px}input,select{width:100%;box-sizing:border-box;background:#0d1117;color:#e8e3d9;border:1px solid #30363d;border-radius:4px;padding:10px;font-size:16px}");
    html += F("input[type=checkbox]{width:auto;transform:scale(1.3);margin-right:8px;accent-color:#c5a059}.row{display:grid;grid-template-columns:1fr 1fr;gap:10px}");
    html += F(".status{display:grid;grid-template-columns:1fr 1fr;gap:8px}.pill{background:#0d1117;border:1px solid #30363d;border-radius:4px;padding:10px}");
    html += F(".ok{color:#c5a059;font-weight:700}.bad{color:#9e2a2b;font-weight:700}.dim{color:#8b949e;font-size:12px}.sens{display:grid;grid-template-columns:1fr auto;gap:6px;align-items:center;padding:10px;background:#0d1117;border:1px solid #30363d;border-radius:4px;margin:6px 0}");
    html += F("button{width:100%;padding:13px 16px;border:0;border-radius:4px;background:#c5a059;color:#0d1117;font-size:17px;font-weight:700;margin-top:16px;cursor:pointer}");
    html += F(".tabs{display:flex;gap:8px;margin:18px 0 0}.tab-btn{width:auto;background:#161b22;color:#8b949e;border:1px solid #30363d;border-radius:4px;padding:11px 16px;font-size:15px;font-weight:600;cursor:pointer;flex:1;text-align:center;margin:0}.tab-btn.active{background:#c5a059;border-color:#c5a059;color:#0d1117}.tab-content{display:none}.tab-content.active{display:block}");
    html += F(".profile{display:grid;grid-template-columns:1fr 96px 82px;gap:8px;align-items:center;background:#0d1117;border:1px solid #30363d;border-radius:4px;padding:9px;margin:8px 0}.profile.active{border-color:#c5a059}.profile strong{display:block}.profile small{color:#8b949e}.profile form{margin:0}.profile button{margin:0;padding:9px 6px;font-size:13px}.profile .danger{background:#9e2a2b}.create-profile{display:grid;grid-template-columns:1fr 150px;gap:10px;align-items:end}.create-profile button{margin:0;height:43px}");
    html += F("a{color:#65b7ff}footer{text-align:center;color:#8b949e;font-size:12px;margin:26px 0 8px}.fork .from{color:#8b949e}");
    html += F("a{color:#65b7ff}@media(max-width:560px){.row,.status,.create-profile{grid-template-columns:1fr}.profile{grid-template-columns:1fr 1fr}.profile>div{grid-column:1/-1}}");
    html += F("</style></head><body><main>");
    html += F("<h1>PirateDriftRC</h1><div class='sub'>Web configurator</div>");

    html += F("<div class='card'><h2>Live Radio</h2><div class='status'>");
    html += F("<div class='pill'>Steering: ");
    html += String(steeringRadio->getPulseWidth());
    html += steeringRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>Gain: ");
    html += String(gainRadio->getPulseWidth());
    html += gainRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    html += F("</div><div class='pill'>Active gain: <strong id='activeGain'>");
    html += gyro != nullptr ? String(gyro->getGain(), 2) : F("--");
    html += F("</strong><br><small id='gainOverride'>Checking gain source...</small>");
    html += F("</div><div class='pill'>Throttle: ");
    html += String(throttleRadio->getPulseWidth());
    html += throttleRadio->hasSignal() ? F(" OK") : F(" NO SIGNAL");
    #if defined(OPENDRIFT_INPUT_CRSF)
    #if defined(OPENDRIFT_CRSF_OOPS_SWAPPED_PINS)
    html += F("</div><div class='pill'>CRSF OOPS: receiver TX to GPIO 17 / RX to GPIO 18");
    #elif defined(OPENDRIFT_AMOLED_V2)
    html += F("</div><div class='pill'>CRSF: GPIO 1 RX / 2 TX");
    #else
    html += F("</div><div class='pill'>CRSF: GPIO 17 RX / 18 TX");
    #endif
    #else
    #if defined(OPENDRIFT_AMOLED_V2)
    html += F("</div><div class='pill'>GPIO 2: ");
    #else
    html += F("</div><div class='pill'>GPIO 18: ");
    #endif
    html += settings->getThrottleOutputEnabled()
        ? F("THROTTLE OUT")
        : F("GAIN INPUT");
    #endif
    html += F("</div></div></div>");
html += F("<div class='card'><h2>Sensors</h2>");

    #if defined(OPENDRIFT_INPUT_CRSF)
    html += F("<div class='sens'><div>ELRS receiver <span class='dim'>ExpressLRS / CRSF</span></div><div id='elrsPill' class='bad'>Checking...</div></div>");
    html += F("<p class='sub'><span class='dim'>Frames:</span> <span id='elrsFrames'>-</span> &middot; <span class='dim'>CRC errors:</span> <span id='elrsCrc'>-</span> &middot; <span class='dim'>Frame age:</span> <span id='elrsAge'>-</span></p>");
    #else
    html += F("<div class='sens'><div>ELRS receiver <span class='dim'>not used (PWM input build)</span></div><div id='elrsPill' class='dim'>N/A</div></div>");
    #endif

    html += F("<div class='sens'><div>Gyro / IMU</div><div id='imuPill' class='bad'>Checking...</div></div>");
    html += F("<p class='sub'><span class='dim'>Gyro X:</span> <span id='gyroX'>-</span> &middot; <span class='dim'>Gyro Y:</span> <span id='gyroY'>-</span> &middot; <span class='dim'>Yaw rate:</span> <span id='yawRate'>-</span> &middot; <span class='dim'>Accel mag:</span> <span id='accelMag'>-</span></p>");
    html += F("</div>");

    html += F("<form method='post' action='/save'>");
    html += F("<div class='tabs'><button type='button' class='tab-btn active' data-tab='tab-tune' onclick='switchTab(\"tab-tune\")'>Tune</button><button type='button' class='tab-btn' data-tab='tab-hw' onclick='switchTab(\"tab-hw\")'>Hardware</button></div>");
    html += F("<script>function switchTab(id){var btns=document.querySelectorAll('.tab-btn');for(var i=0;i<btns.length;i++){btns[i].classList.toggle('active',btns[i].getAttribute('data-tab')===id);}var cts=document.querySelectorAll('.tab-content');for(var j=0;j<cts.length;j++){cts[j].classList.toggle('active',cts[j].id===id);}}function postForm(action,data){var f=document.createElement('form');f.method='post';f.action=action;f.style.display='none';for(var k in data){var el=document.createElement('input');el.type='hidden';el.name=k;el.value=data[k];f.appendChild(el);}document.body.appendChild(f);f.submit();}function activateProfile(i){postForm('/activate-profile',{'profile':String(i)});}function deleteProfile(i){if(!confirm('Delete this profile?'))return;postForm('/delete-profile',{'profile':String(i)});}function createProfile(){var inp=document.querySelector('.create-profile input[name=name]');var v=inp?inp.value.trim():'';if(!v)return;postForm('/create-profile',{'name':v});}function clearLog(){postForm('/clear-log',{});}</script>");
    html += F("<div id='tab-tune' class='tab-content active'>");

    html += F("<div class='card'><h2>Driving Profiles</h2><p class='sub'>Active: <strong>");
    html += settings->getActiveProfileName();
    html += F("</strong>. Active profiles automatically keep trackside tune changes.</p>");

    for(uint8_t i = 0; i < settings->getProfileCount(); i++)
    {
        const Settings::DrivingProfile* profile =
            settings->getProfile(i);

        if(profile == nullptr)
        {
            continue;
        }

        html += F("<div class='profile");

        if(settings->getActiveProfileIndex() == i)
        {
            html += F(" active");
        }

        html += F("'><div><strong>");
        html += profile->name;
        html += F("</strong><small>Gain ");
        html += String(profile->gain, 2);
        html += F(" &middot; Prediction ");
        html += String(profile->predictionStrength);
        html += F(" &middot; Hold ");
        html += String(profile->gyroHoldBoost);
        html += F(" &middot; Countersteer ");
        html += String(profile->gyroCounterSteerAssist);
        html += F(" &middot; Transition speed ");
        html += String(profile->gyroTransitionSpeed);
        html += F(" &middot; Anti Wobble ");
        html += String(profile->gyroHuntStrength);
        html += F("</small></div>");

        html += F("<button type='button' onclick='activateProfile(");
        html += String(i);
        html += F(")'>Activate</button>");

        html += F("<button class='danger' type='button' onclick='deleteProfile(");
        html += String(i);
        html += F(")'>Delete</button></div>");
    }

    if(settings->getProfileCount() < Settings::MAX_PROFILES)
    {
        html += F("<div class='create-profile'><div><label>New profile name</label><input name='name' type='text' maxlength='23' required placeholder='Example: P-tile'></div><button type='button' onclick='createProfile()'>Create from current tune</button></div>");
    }
    else
    {
        html += F("<p class='sub'>Profile limit reached. Delete one to create another.</p>");
    }

    html += F("</div>");

    html += F("<div class='card'><h2>Drive &amp; Limits</h2><div class='row'>");
    html += input("Saved gain (fallback)", "gain", String(settings->getGain(), 2), "number", "0.01");
    html += input("Deadband", "deadband", String(settings->getDeadband(), 2), "number", "1");
    html += input("Max correction (% full steering span)", "gyroMax", String(settings->getGyroMaxCorrection()), "number", "1");
    html += F("<p class='sub'>This is the gyro's maximum endpoint-to-endpoint authority. 50% can move from center to one calibrated endpoint; 100% can override one endpoint all the way to the other. Physical endpoint calibration remains the final hard limit.</p>");
    html += F("</div>");
    html += checkbox("Reverse gyro correction", "gyroReverse", settings->getGyroReverse());
    html += F("</div>");

    html += F("<div class='card'><h2>PirateDrift v1.0 Response</h2><div class='row'>");
    html += input("Smoothing", "gyroSmoothing", String(settings->getGyroSmoothing(), 2), "number", "0.01");
    html += F("<label>Gyro sensor LPF</label><select name='gyroLpfMode'><option value='0'");
    if(settings->getGyroLpfMode() == 0) html += F(" selected");
    html += F(">24 Hz - original</option><option value='1'");
    if(settings->getGyroLpfMode() == 1) html += F(" selected");
    html += F(">120 Hz - low latency</option><option value='2'");
    if(settings->getGyroLpfMode() == 2) html += F(" selected");
    html += F(">Off - raw bandwidth</option></select>");
    html += input("Prediction strength (0-100)", "predictionStrength", String(settings->getPredictionStrength()), "number", "1");
    html += input("Anti Wobble (0-100)", "huntStrength", String(settings->getGyroHuntStrength()), "number", "1");
    html += F("<p class='sub'>Anti Wobble controls the depth of PirateDrift's narrow, phase-aware wheel-wobble notch. Start at 50. Raise it only if a repeating wheel oscillation remains; lower it if steering begins to feel soft or unnatural. Zero bypasses the notch and 100 applies its maximum depth.</p>");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Transition Response</h2><p class='sub'>Transition Speed follows the complete chassis direction change. 50 is neutral; lower values add damping for slower transitions and higher values release damping for faster transitions. It never changes the Max Correction ceiling. Compare 25, 50, and 75 at the same tune.</p><div class='row'>");
    html += input("Transition speed (0-100)", "transitionSpeed", String(settings->getGyroTransitionSpeed()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Drift Assist</h2><p class='sub'>Countersteer Assist changes only the steady steering workload. Zero preserves the base v1.0 response; higher values let PirateDrift carry more of a settled drift.</p><div class='row'>");
    html += input("Countersteer assist (0-100)", "counterSteerAssist", String(settings->getGyroCounterSteerAssist()), "number", "1");
    html += input("Hold assist (0-100)", "gyroHoldBoost", String(settings->getGyroHoldBoost()), "number", "1");
    html += input("Drift memory", "gyroIGain", String(settings->getGyroIntegralGain(), 2), "number", "0.01");
    html += input("Memory limit (us)", "gyroILimit", String(settings->getGyroIntegralLimit()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>WiFi</h2>");
    html += checkbox("Enable WiFi on boot", "wifiEnabled", settings->getWifiEnabled());
    html += input("Auto-off timeout ms", "wifiTimeout", String(settings->getWifiTimeout()));
    html += F("<p class='sub'>Auto-off counts only while no device is connected. A connected phone pauses the timer; a disconnect starts a fresh timeout.</p>");
    html += F("</div>");

    html += F("<div class='card'><h2>Blackbox</h2>");
    html += checkbox("Enable onboard logging", "blackboxEnabled", settings->getBlackboxEnabled());
    html += F("</div>");

    html += F("</div>");
    html += F("<div id='tab-hw' class='tab-content'>");

    html += F("<div class='card'><h2>Servo</h2>");
    html += checkbox("Reverse servo", "servoReverse", settings->getServoReverse());
    html += F("<label>Control and servo rate</label><select name='controlLoopHz'><option value='250'");
    if(settings->getControlLoopHz() == 250) html += F(" selected");
    html += F(">250 Hz - broad servo compatibility</option><option value='333'");
    if(settings->getControlLoopHz() == 333) html += F(" selected");
    html += F(">333 Hz - supported servos only</option></select><p class='sub'>250 Hz supports a broader range of digital servos. Select 333 Hz only when the servo manufacturer explicitly supports it. A restart is required after changing this setting.</p>");
    html += F("<div class='row'>");
    html += input("Center pulse", "servoCenter", String(settings->getServoCenter()));
    html += input("Travel percent", "servoTravel", String(settings->getServoTravel()));
    html += input("Quiet band us", "servoQuiet", String(settings->getServoQuiet()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Physical Servo Endpoints</h2><p class='sub'>Status: <strong>");
    html += settings->isSteeringCalibrated() ? F("CALIBRATED") : F("NOT CALIBRATED");
    html += F("</strong>. These are the servo's physical PWM stops and the final hard limits for both driver and gyro movement. Position the wheels at each safe physical endpoint and capture it from the display or EdgeTX tool, or enter all three pulse values below.</p><div class='row'>");
    html += input("Max left", "steeringMin", String(settings->getSteeringMin()));
    html += input("Center", "steeringCenter", String(settings->getSteeringCenter()));
    html += input("Max right", "steeringMax", String(settings->getSteeringMax()));
    html += input("Steering travel percent", "radioSteeringTravel", String(settings->getRadioSteeringTravel()), "number", "1");
    html += F("</div></div>");

    html += F("<div class='card'><h2>Gain Channel Calibration</h2><div class='row'>");
    #if defined(OPENDRIFT_INPUT_CRSF)
    #if defined(OPENDRIFT_CRSF_OOPS_SWAPPED_PINS)
    html += F("Personal swapped-pin build: CRSF channel 3 controls gyro gain. GPIO 16 drives the steering servo. GPIO 15 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold. Receiver TX feeds GPIO 17; receiver RX connects to GPIO 18.");
    #elif defined(OPENDRIFT_AMOLED_V2)
    html += F("CRSF channel 3 controls gyro gain. GPIO 15 drives the steering servo. GPIO 16 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold. Receiver TX feeds GPIO 1; receiver RX connects to GPIO 2.");
    #else
    html += F("CRSF channel 3 controls gyro gain. GPIO 15 drives the steering servo. GPIO 16 actively outputs neutral throttle during failsafe and passes throttle only after a valid neutral hold.");
    #endif
    html += F("</div>");
    #else
    html += input("Gain low", "gainMin", String(settings->getGainMin()));
    html += input("Gain high", "gainMax", String(settings->getGainMax()));
    html += F("</div>");
    html += checkbox(
        #if defined(OPENDRIFT_AMOLED_V2)
        "Use GPIO 2 as throttle output instead of gyro gain input",
        #else
        "Use GPIO 18 as throttle output instead of gyro gain input",
        #endif
        "throttleOutputEnabled",
        settings->getThrottleOutputEnabled()
    );
    #endif
    html += F("<div class='row'>");
    html += input("CH3 gain minimum", "channel3GainMin", String(settings->getChannel3GainMin(), 2), "number", "0.05");
    html += input("CH3 gain maximum", "channel3GainMax", String(settings->getChannel3GainMax(), 2), "number", "0.05");
    html += F("</div><p class='sub'>Maps the full Channel 3 control range to gyro gain. Defaults are 0.50 to 3.00; both ends support 0.00 to 6.00.</p>");
    html += F("</div>");

    #if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
    html += F("<div class='card'><h2>Auxiliary Channel Outputs</h2><p class='sub'>Route any CRSF channel to a standard 50 Hz receiver-style PWM signal. Outputs return to 1500 us on signal loss. GPIO is 3.3 V signal only: power accessories externally and connect a common ground.</p><div class='row'>");

    for(uint8_t gpio = 1; gpio <= 8; gpio++)
    {
        html += F("<div><label>GPIO ");
        html += String(gpio);

        if(!AuxChannelOutputs::isPinAvailable(gpio))
        {
            html += F("</label><div class='pill'>Reserved for CRSF UART</div></div>");
            continue;
        }

        html += F("</label><select name='auxGpio");
        html += String(gpio);
        html += F("'><option value='0'");

        uint8_t selectedChannel =
            settings->getAuxChannelForGpio(gpio);

        if(selectedChannel == 0)
        {
            html += F(" selected");
        }

        html += F(">Disabled</option>");

        for(uint8_t channel = 1; channel <= 16; channel++)
        {
            html += F("<option value='");
            html += String(channel);
            html += F("'");

            if(selectedChannel == channel)
            {
                html += F(" selected");
            }

            html += F(">CRSF Channel ");
            html += String(channel);
            html += F("</option>");
        }

        html += F("</select></div>");
    }

    html += F("</div><p class='sub'>Mappings take effect immediately after Save Settings. Multiple GPIOs may mirror the same channel.</p></div>");
    #endif

    html += F("<div class='card'><h2>Hardware Test</h2><p class='sub'>Sweeps the servo or motor between endpoints to verify wiring. The gyro/radio outputs are suppressed while a test runs. Keep the wheels off the ground and the propeller clear.</p>");
    html += F("<label><input type='checkbox' id='hwUnlock' onchange='syncHwUnlock()'> <strong>Unlock hardware test</strong> <span class='dim'>(tick to enable the test buttons)</span></label>");
    html += F("<div class='row' style='margin-top:12px'><button type='button' id='btnTestServo' onclick='runHwTest(\"servo\")'>Test Servo</button>");
    html += F("<button type='button' id='btnTestMotor' onclick='runHwTest(\"motor\")'>Test Motor</button></div>");
    html += F("<p class='sub' id='hwStatus'>Idle</p>");
    html += F("<script>function syncHwUnlock(){var u=document.getElementById('hwUnlock').checked;document.getElementById('btnTestServo').disabled=!u;document.getElementById('btnTestMotor').disabled=!u;}function runHwTest(t){if(!document.getElementById('hwUnlock').checked)return;var st=document.getElementById('hwStatus');st.textContent='Running '+t+' test (moves servo/motor)...';st.style.color='#f0c24b';fetch('/api/test_'+t+'?unlock=1',{method:'POST',cache:'no-store'}).then(function(r){return r.text();}).then(function(x){st.textContent=x;st.style.color='#8b949e';}).catch(function(){st.textContent='Request failed';st.style.color='#9e2a2b';});}syncHwUnlock();</script></div>");

    html += F("<div class='card'><h2>Blackbox Log</h2>");

    if(!settings->getBlackboxEnabled())
    {
        html += F("<p class='sub'>Logging disabled. Enable onboard logging and save settings to record logs.</p>");
    }
    else if(blackbox != nullptr && blackbox->isReady())
    {
        html += F("<p class='sub'>Binary records in PSRAM: ");
        html += String(blackbox->getRecordCount());
        html += F(" &middot; used: ");
        html += String(blackbox->getSize() / 1024);
        html += F(" / ");
        html += String(blackbox->getCapacityBytes() / 1024);
        html += F(" KB &middot; duration: ");
        html += String(blackbox->getDurationMs() / 60000);
        html += F("m ");
        html += String((blackbox->getDurationMs() / 1000) % 60);
        html += F("s");

        if(blackbox->isFull())
        {
            html += F(" &middot; retaining newest records");
        }

        if(blackbox->getOverwrittenRows() > 0)
        {
            html += F(" &middot; overwritten: ");
            html += String(blackbox->getOverwrittenRows());
        }

        html += F("</p><p class='sub'>Stage-one logger: records stay entirely in volatile PSRAM. No internal flash writes occur. Download converts the binary records to CSV; power cycling clears the log.</p>");
        html += F("<a href='/blackbox.csv'>Download CSV</a>");
        html += F("<button type='button' onclick='clearLog()'>Clear RAM Log</button>");
    }
    else
    {
        html += F("<p class='sub'>PSRAM log buffer unavailable.</p>");
    }

    html += F("</div>");

    html += F("</div>");
    html += F("<button type='submit'>Save Settings</button></form>");

    html += F("</main><footer class='fork'>PirateDriftRC &mdash; a fork of <span class='from'>OpenDriftRC</span></footer><script>function updateLive(){fetch('/live-status',{cache:'no-store'}).then(r=>r.json()).then(s=>{document.getElementById('activeGain').textContent=Number(s.gain).toFixed(2);document.getElementById('gainOverride').textContent=s.override?'CH3 gain override active':'Saved gain active';function setPill(id,txt,ok){var el=document.getElementById(id);if(!el)return;el.textContent=txt;el.className=ok?'ok':'bad';}if(s.elrs&&s.elrs.present){setPill('elrsPill',s.elrs.connected?('Connected'+(s.elrs.lq>0?' &middot; LQ '+s.elrs.lq+'%':'')+(s.elrs.snr!==0?' &middot; '+s.elrs.snr+'dB':'')):'No signal',s.elrs.connected);var f=document.getElementById('elrsFrames');if(f)f.textContent=s.elrs.frames;var c=document.getElementById('elrsCrc');if(c){c.textContent=s.elrs.crcErrors;c.className=s.elrs.crcErrors>0?'bad':'dim';}var a=document.getElementById('elrsAge');if(a)a.textContent=s.elrs.frameAgeMs+' ms';}else{setPill('elrsPill','N/A',true);}if(s.imu){setPill('imuPill',s.imu.ready?'Ready':'Module not found',s.imu.ready);var gx=document.getElementById('gyroX');if(gx)gx.textContent=s.imu.gyroX;var gy=document.getElementById('gyroY');if(gy)gy.textContent=s.imu.gyroY;var yr=document.getElementById('yawRate');if(yr)yr.textContent=s.imu.yawRate;var am=document.getElementById('accelMag');if(am)am.textContent=s.imu.accelMag;}}).catch(()=>{});}updateLive();setInterval(updateLive,500);</script></body></html>");

    server.send(
        200,
        "text/html",
        html
    );
}


void WebConfigurator::handleLiveStatus()
{
    if(
        settings == nullptr ||
        gyro == nullptr ||
        gainRadio == nullptr
    )
    {
        server.send(
            503,
            "application/json",
            "{\"error\":\"unavailable\"}"
        );

        return;
    }

    bool gainOverride = gainRadio->hasSignal();

    #if !defined(OPENDRIFT_INPUT_CRSF)
    gainOverride =
        gainOverride &&
        !settings->getThrottleOutputEnabled();
    #endif

    String json;
    json.reserve(360);
    json += F("{\"gain\":");
    json += String(gyro->getGain(), 2);
    json += F(",\"pulse\":");
    json += String(gainRadio->getPulseWidth());
    json += F(",\"override\":");
    json += gainOverride ? F("true") : F("false");

    // IMU / gyro sensor status
    json += F(",\"imu\":{");
    bool imuReady = (imu != nullptr) && imuOk && imu->isReady();
    json += F("\"ready\":");
    json += imuReady ? F("true") : F("false");
    json += F(",\"gyroX\":");
    json += imuReady ? String(imu->getGyroX(), 2) : String(0.0f, 2);
    json += F(",\"gyroY\":");
    json += imuReady ? String(imu->getGyroY(), 2) : String(0.0f, 2);
    json += F(",\"yawRate\":");
    json += imuReady ? String(imu->getYawRate(), 2) : String(0.0f, 2);
    json += F(",\"accelMag\":");
    json += imuReady ? String(imu->getAccelMagnitude(), 2) : String(0.0f, 2);
    json += F("}");

    // CRSF / ELRS receiver status
    #if defined(OPENDRIFT_INPUT_CRSF)
    bool crsfPresent = (crsf != nullptr);
    bool crsfConnected = crsfPresent && crsf->hasSignal();
    json += F(",\"elrs\":{");
    json += F("\"present\":");
    json += crsfPresent ? F("true") : F("false");
    json += F(",\"connected\":");
    json += crsfConnected ? F("true") : F("false");
    json += F(",\"frameAgeMs\":");
    json += crsfPresent ? String(crsf->getFrameAgeMs()) : String(0);
    json += F(",\"lq\":");
    json += crsfConnected ? String((int)crsf->getUplinkLinkQuality()) : String(0);
    json += F(",\"snr\":");
    json += crsfConnected ? String((int)crsf->getUplinkSnr()) : String(0);
    json += F(",\"frames\":");
    json += crsfPresent ? String(crsf->getValidFrameCount()) : String(0);
    json += F(",\"crcErrors\":");
    json += crsfPresent ? String(crsf->getCrcErrorCount()) : String(0);
    json += F("}");
    #else
    json += F(",\"elrs\":null");
    #endif

    json += F("}");

    server.sendHeader(
        "Cache-Control",
        "no-store"
    );

    server.send(
        200,
        "application/json",
        json
    );
}


void WebConfigurator::handleSave()
{
    if(settings == nullptr)
    {
        server.send(
            503,
            "text/plain",
            "Settings unavailable"
        );

        return;
    }

    settings->setGain(
        getFloatArg(
            "gain",
            settings->getGain()
        )
    );

    settings->setDeadband(
        getFloatArg(
            "deadband",
            settings->getDeadband()
        )
    );

    settings->setGyroReverse(
        server.hasArg("gyroReverse")
    );

    settings->setGyroMaxCorrection(
        getIntArg(
            "gyroMax",
            settings->getGyroMaxCorrection()
        )
    );

    settings->setGyroSmoothing(
        getFloatArg(
            "gyroSmoothing",
            settings->getGyroSmoothing()
        )
    );

    settings->setGyroLpfMode(
        getIntArg(
            "gyroLpfMode",
            settings->getGyroLpfMode()
        )
    );

    settings->setGyroIntegralGain(
        getFloatArg(
            "gyroIGain",
            settings->getGyroIntegralGain()
        )
    );

    settings->setGyroIntegralLimit(
        getIntArg(
            "gyroILimit",
            settings->getGyroIntegralLimit()
        )
    );

    settings->setGyroHoldBoost(
        getIntArg(
            "gyroHoldBoost",
            settings->getGyroHoldBoost()
        )
    );

    settings->setGyroCounterSteerAssist(
        getIntArg(
            "counterSteerAssist",
            settings->getGyroCounterSteerAssist()
        )
    );

    settings->setGyroTransitionSpeed(
        getIntArg(
            "transitionSpeed",
            settings->getGyroTransitionSpeed()
        )
    );

    settings->setPredictionStrength(
        getIntArg(
            "predictionStrength",
            settings->getPredictionStrength()
        )
    );

    settings->setGyroHuntStrength(
        getIntArg(
            "huntStrength",
            settings->getGyroHuntStrength()
        )
    );

    settings->setServoReverse(
        server.hasArg("servoReverse")
    );

    settings->setServoCenter(
        getIntArg(
            "servoCenter",
            settings->getServoCenter()
        )
    );

    settings->setServoTravel(
        getIntArg(
            "servoTravel",
            settings->getServoTravel()
        )
    );

    settings->setServoQuiet(
        getIntArg(
            "servoQuiet",
            settings->getServoQuiet()
        )
    );

    settings->setControlLoopHz(
        getIntArg(
            "controlLoopHz",
            settings->getControlLoopHz()
        )
    );

    int requestedSteeringMin =
        getIntArg(
            "steeringMin",
            settings->getSteeringMin()
        );

    int requestedSteeringCenter =
        getIntArg(
            "steeringCenter",
            settings->getSteeringCenter()
        );

    int requestedSteeringMax =
        getIntArg(
            "steeringMax",
            settings->getSteeringMax()
        );

    bool steeringCalibrationChanged =
        requestedSteeringMin != settings->getSteeringMin() ||
        requestedSteeringCenter != settings->getSteeringCenter() ||
        requestedSteeringMax != settings->getSteeringMax();

    settings->setSteeringMin(requestedSteeringMin);
    settings->setSteeringCenter(requestedSteeringCenter);
    settings->setSteeringMax(requestedSteeringMax);

    if(steeringCalibrationChanged)
    {
        settings->confirmStoredSteeringCalibration();
    }

    settings->setRadioSteeringTravel(
        getIntArg(
            "radioSteeringTravel",
            settings->getRadioSteeringTravel()
        )
    );

    settings->setGainMin(
        getIntArg(
            "gainMin",
            settings->getGainMin()
        )
    );

    settings->setGainMax(
        getIntArg(
            "gainMax",
            settings->getGainMax()
        )
    );

    settings->setChannel3GainMin(
        getFloatArg(
            "channel3GainMin",
            settings->getChannel3GainMin()
        )
    );

    settings->setChannel3GainMax(
        getFloatArg(
            "channel3GainMax",
            settings->getChannel3GainMax()
        )
    );

    #if !defined(OPENDRIFT_INPUT_CRSF)
    settings->setThrottleOutputEnabled(
        server.hasArg("throttleOutputEnabled")
    );
    #endif

    #if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
    for(uint8_t gpio = 1; gpio <= 8; gpio++)
    {
        if(!AuxChannelOutputs::isPinAvailable(gpio))
        {
            continue;
        }

        char argument[12];

        snprintf(
            argument,
            sizeof(argument),
            "auxGpio%u",
            gpio
        );

        settings->setAuxChannelForGpio(
            gpio,
            getIntArg(
                argument,
                settings->getAuxChannelForGpio(gpio)
            )
        );
    }
    #endif

    settings->setWifiEnabled(
        server.hasArg("wifiEnabled")
    );

    settings->setWifiTimeout(
        getIntArg(
            "wifiTimeout",
            settings->getWifiTimeout()
        )
    );

    settings->setBlackboxEnabled(
        server.hasArg("blackboxEnabled")
    );

    if(gyro != nullptr)
    {
        gyro->setGain(
            settings->getGain()
        );

        gyro->setDeadband(
            settings->getDeadband()
        );

        gyro->setSmoothing(
            settings->getGyroSmoothing()
        );

        gyro->setMaxCorrection(
            settings->getGyroMaxCorrection() * 10
        );

        gyro->setIntegralGain(
            settings->getGyroIntegralGain()
        );

        gyro->setIntegralLimit(
            settings->getGyroIntegralLimit()
        );

        gyro->setHoldBoost(
            settings->getGyroHoldBoost()
        );

        gyro->setCounterSteerAssist(
            settings->getGyroCounterSteerAssist()
        );

        gyro->setTransitionSpeed(
            settings->getGyroTransitionSpeed()
        );

        gyro->setPredictionStrength(
            settings->getPredictionStrength()
        );

        gyro->setHuntStrength(
            settings->getGyroHuntStrength()
        );
    }

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}



void WebConfigurator::handleProfileCreate()
{
    if(
        settings == nullptr ||
        !server.hasArg("name")
    )
    {
        server.send(
            400,
            "text/plain",
            "Profile name required"
        );

        return;
    }

    if(settings->createProfile(server.arg("name")) < 0)
    {
        server.send(
            400,
            "text/plain",
            "Could not create profile. Use a unique name and check the profile limit."
        );

        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}


void WebConfigurator::handleProfileActivate()
{
    if(
        settings == nullptr ||
        !server.hasArg("profile")
    )
    {
        server.send(400, "text/plain", "Profile required");
        return;
    }

    int index = server.arg("profile").toInt();

    if(
        index < 0 ||
        index >= settings->getProfileCount() ||
        !settings->activateProfile(index)
    )
    {
        server.send(404, "text/plain", "Profile not found");
        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}


void WebConfigurator::handleProfileDelete()
{
    if(
        settings == nullptr ||
        !server.hasArg("profile")
    )
    {
        server.send(400, "text/plain", "Profile required");
        return;
    }

    int index = server.arg("profile").toInt();

    if(
        index < 0 ||
        index >= settings->getProfileCount() ||
        !settings->deleteProfile(index)
    )
    {
        server.send(404, "text/plain", "Profile not found");
        return;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}



void WebConfigurator::handleLogDownload()
{
    if(
        settings != nullptr &&
        !settings->getBlackboxEnabled()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox logging disabled"
        );

        return;
    }

    if(
        blackbox == nullptr ||
        !blackbox->isReady()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox log unavailable"
        );

        return;
    }

    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=opendrift-blackbox.csv"
    );

    server.setContentLength(
        CONTENT_LENGTH_UNKNOWN
    );

    server.send(
        200,
        "text/csv",
        ""
    );

    server.sendContent(
        blackbox->getCsvHeader()
    );
    server.sendContent("\n");

    size_t recordCount =
        blackbox->getRecordCount();

    char line[672];
    String chunk;
    chunk.reserve(8192);

    for(size_t index = 0; index < recordCount; index++)
    {
        size_t length =
            blackbox->formatCsvRecord(
                index,
                line,
                sizeof(line)
            );

        if(length == 0)
        {
            continue;
        }

        if(chunk.length() + length > 8192)
        {
            server.sendContent(chunk);
            chunk = "";

            if(!server.client().connected())
            {
                return;
            }
        }

        chunk.concat(line, length);

        if((index & 0x7F) == 0)
        {
            delay(0);
        }
    }

    if(chunk.length() > 0)
    {
        server.sendContent(chunk);
    }

    server.sendContent("");
}



void WebConfigurator::handleLogClear()
{
    if(
        settings != nullptr &&
        !settings->getBlackboxEnabled()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox logging disabled"
        );

        return;
    }

    if(
        blackbox == nullptr ||
        !blackbox->isReady()
    )
    {
        server.send(
            503,
            "text/plain",
            "Blackbox log unavailable"
        );

        return;
    }

    blackbox->clear();

    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        303
    );
}



// Hardware test task parameter bundle. Kept small and self-contained so the
// task has no dependency on volatile WebConfigurator members beyond the flag.
struct HardwareTestParam
{
    WebConfigurator* self = nullptr;

    ServoOutput* servo = nullptr;

    EscOutput* esc = nullptr;

    volatile bool* flag = nullptr;

    uint8_t motorPin = 0;

    bool motor = false;
};



void WebConfigurator::handleTestServo()
{
    startHardwareTest(false);
}



void WebConfigurator::handleTestMotor()
{
    startHardwareTest(true);
}



void WebConfigurator::startHardwareTest(
    bool motor
)
{
    if(!server.hasArg("unlock"))
    {
        server.send(
            403,
            "text/plain",
            "Hardware test blocked: unlock flag missing"
        );

        return;
    }

    if(hwTestRunning)
    {
        server.send(
            409,
            "text/plain",
            "Hardware test already running"
        );

        return;
    }

    HardwareTestParam* param =
        new HardwareTestParam();

    param->self = this;
    param->servo = servoOut;
    param->esc = escOut;
    param->flag = hardwareTestFlag;
    param->motorPin = motorOutputPin;
    param->motor = motor;

    BaseType_t created =
        xTaskCreate(
            hardwareTestTask,
            "hwTest",
            4096,
            param,
            2,
            &hwTestTaskHandle
        );

    if(created != pdPASS)
    {
        delete param;

        server.send(
            500,
            "text/plain",
            "Failed to start hardware test task"
        );

        return;
    }

    hwTestRunning = true;

    server.send(
        200,
        "text/plain",
        motor
            ? "Motor test started"
            : "Servo test started"
    );
}



void WebConfigurator::hardwareTestTask(
    void* paramPtr
)
{
    HardwareTestParam* param =
        static_cast<HardwareTestParam*>(paramPtr);

    if(param->flag != nullptr)
    {
        *param->flag = true;
    }

    // Fully sweep the output between its mechanical endpoints: neutral (1500)
    // up to max (2000), pause, down to min (1000), pause, and back to neutral.
    constexpr int STEP_US = 25;
    constexpr int STEP_DELAY_MS = 15;

    if(!param->motor)
    {
        ServoOutput* servo = param->servo;

        if(servo != nullptr)
        {
            for(int us = 1500; us <= 2000; us += STEP_US)
            {
                servo->writeMicroseconds(us);
                vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
            }

            vTaskDelay(pdMS_TO_TICKS(500));

            for(int us = 2000; us >= 1000; us -= STEP_US)
            {
                servo->writeMicroseconds(us);
                vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
            }

            vTaskDelay(pdMS_TO_TICKS(500));

            for(int us = 1000; us <= 1500; us += STEP_US)
            {
                servo->writeMicroseconds(us);
                vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
            }
        }
    }
    else
    {
        EscOutput* esc = param->esc;

        if(esc != nullptr)
        {
            // Take sole ownership of the ESC output for the duration of the
            // test, then restore a clean detached state.
            esc->end();
            esc->configure(1500, false, 100, 0);

            if(esc->begin(param->motorPin, 50))
            {
                for(int us = 1500; us <= 2000; us += STEP_US)
                {
                    esc->writeMicroseconds(us);
                    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
                }

                vTaskDelay(pdMS_TO_TICKS(500));

                for(int us = 2000; us >= 1000; us -= STEP_US)
                {
                    esc->writeMicroseconds(us);
                    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
                }

                vTaskDelay(pdMS_TO_TICKS(500));

                for(int us = 1000; us <= 1500; us += STEP_US)
                {
                    esc->writeMicroseconds(us);
                    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
                }

                esc->end();
            }
        }
    }

    if(param->flag != nullptr)
    {
        *param->flag = false;
    }

    if(param->self != nullptr)
    {
        param->self->hwTestRunning = false;
    }

    delete param;

    vTaskDelete(
        nullptr
    );
}



void WebConfigurator::handleNotFound()
{
    server.sendHeader(
        "Location",
        "/"
    );

    server.send(
        302
    );
}



String WebConfigurator::input(
    const char* label,
    const char* name,
    String value,
    const char* type,
    const char* step
)
{
    String html;

    html += F("<div><label>");
    html += label;
    html += F("</label><input name='");
    html += name;
    html += F("' type='");
    html += type;
    html += F("' step='");
    html += step;
    html += F("' value='");
    html += value;
    html += F("'></div>");

    return html;
}



String WebConfigurator::checkbox(
    const char* label,
    const char* name,
    bool checked
)
{
    String html;

    html += F("<label><input name='");
    html += name;
    html += F("' type='checkbox'");

    if(checked)
    {
        html += F(" checked");
    }

    html += F(">");
    html += label;
    html += F("</label>");

    return html;
}



int WebConfigurator::getIntArg(
    const char* name,
    int fallback
)
{
    if(!server.hasArg(name))
    {
        return fallback;
    }

    return server.arg(name).toInt();
}



float WebConfigurator::getFloatArg(
    const char* name,
    float fallback
)
{
    if(!server.hasArg(name))
    {
        return fallback;
    }

    return server.arg(name).toFloat();
}
