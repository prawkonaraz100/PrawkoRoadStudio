// SPDX-License-Identifier: Apache-2.0
// Four original PrawkoNaRaz road scenarios; does not read or reuse old scenes.
const fs=require('fs'), path=require('path');
const folder=path.resolve(__dirname,'..','projects','Prawko-Scenariusze');
if(fs.existsSync(folder)) throw new Error('Existing destination: cannot overwrite user files');
fs.mkdirSync(folder,{recursive:true});
const xml=t=>String(t).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;');
const tag=(name,attr,body)=>'<'+name+(attr?' '+attr:'')+'>'+(body||'')+'</'+name+'>';
const attrs=o=>Object.entries(o).map(([key,val])=>key+'="'+xml(val)+'"').join(' ');
function lane(id,type,width,mark) {
 let contents='<width sOffset="0" a="'+width+'" b="0" c="0" d="0" />';
 if(mark)contents+='<roadMark sOffset="0" type="'+mark+'" color="white" width="0.12" />';
 return tag('lane',attrs({id,type,level:'false'}),contents);
}
function zebra(s) {
 const xa=(s-1.5).toFixed(2),xb=(s+1.5).toFixed(2);
 const points=[[xa,-5.6],[xb,-5.6],[xb,3.4],[xa,3.4]];
 const corners=points.map(([x,y],i)=>'<cornerRoad id="'+i+'" s="'+x+'" t="'+y+'" dz="0" height="0"/>').join('');
 const refs=points.map((_,i)=>'<cornerReference id="'+i+'"/>').join('');
 return '<object type="crosswalk" subtype="zebra" id="1" name="Zebra Prawko" s="'+s+'" t="-1.1" zOffset="0" orientation="none" hdg="1.5707963267948966" length="9" width="3">'
  +'<outlines><outline id="0" fillType="paint" outer="true" closed="true">'+corners+'</outline></outlines>'
  +'<markings><marking color="white" width="0.1" zOffset="0.005" spaceLength="0.5" lineLength="0.5" startOffset="0" stopOffset="0">'
  +refs+'</marking></markings><userData code="rm:crosswalk" dashLength="0.5" dashGap="0.5"/></object>';
}
function road(s) {
 const roadType=s.rural?'rural':'townArterial';
 const geo=s.curve ? '<geometry s="0" x="0" y="0" hdg="0" length="90"><line/></geometry>'
  +'<geometry s="90" x="90" y="0" hdg="0" length="'+(s.length-90)+'"><arc curvature="0.003"/></geometry>'
  :'<geometry s="0" x="0" y="0" hdg="0" length="'+s.length+'"><line/></geometry>';
 const left=lane(2,s.rural?'shoulder':'sidewalk',s.rural?1.6:2.1)+lane(1,'driving',3.4,'solid');
 const right=lane(-1,'driving',3.4,'solid')+(s.rural?lane(-2,'shoulder',1.8):lane(-2,'parking',2.2)+lane(-3,'sidewalk',2.1));
 const name=xml(s.title);
 return ['<?xml version="1.0" encoding="UTF-8"?>',
 '<!-- Original PrawkoNaRaz road. Apache-2.0, authored 2026-11-15. -->',
 '<OpenDRIVE>',
 '<header revMajor="1" revMinor="8" name="'+name+'" vendor="PrawkoNaRaz"/>',
 '<road name="'+name+'" length="'+s.length+'" id="1" junction="-1">',
 '<type s="0" type="'+roadType+'"><speed max="'+s.speedLimit+'" unit="km/h"/></type>',
 '<planView>'+geo+'</planView>',
 '<elevationProfile><elevation s="0" a="0" b="0" c="0" d="0"/></elevationProfile>',
 '<lanes><laneSection s="0"><left>'+left+'</left>',
 '<center><lane id="0" type="none"><roadMark sOffset="0" type="broken" color="white" width="0.12"/></lane></center>',
 '<right>'+right+'</right></laneSection></lanes>',
 s.zebra?'<objects>'+zebra(s.zebra)+'</objects>':'',
 '<userData code="rm:waypoints" value="'+(s.curve?'0,0;90,0':'0,0;'+s.length+',0')+'"/>',
 '</road></OpenDRIVE>',''].join('\n');
}
function vehicle(name) {
 return '<ScenarioObject name="'+xml(name)+'"><Vehicle name="PrawkoCar" vehicleCategory="car" mass="1450">'
 +'<BoundingBox><Center x="1.75" y="0" z="0.72"/><Dimensions width="1.83" length="4.58" height="1.44"/></BoundingBox>'
 +'<Performance maxSpeed="55" maxAcceleration="4" maxDeceleration="9"/>'
 +'<Axles><FrontAxle maxSteering="0.5" wheelDiameter="0.64" trackWidth="1.60" positionX="2.75" positionZ="0.32"/>'
 +'<RearAxle maxSteering="0" wheelDiameter="0.64" trackWidth="1.60" positionX="0" positionZ="0.32"/></Axles>'
 +'<Properties/></Vehicle></ScenarioObject>';
}
function pedestrian(){
 return '<ScenarioObject name="Pieszy"><Pedestrian name="Prawko Pedestrian" pedestrianCategory="pedestrian" mass="76" model="">'
 +'<BoundingBox><Center x="0" y="0" z="0.85"/><Dimensions width="0.55" length="0.40" height="1.70"/></BoundingBox>'
 +'<Properties/></Pedestrian></ScenarioObject>';
}
function lanePosition(s){return '<LanePosition roadId="1" laneId="-1" s="'+s+'" offset="0"/>';}
function worldPosition(x,y,z){return '<WorldPosition x="'+x+'" y="'+y+'" z="'+(z||0.15)+'" h="1.5707963267948966"/>';}
function position(inner){return '<Position>'+inner+'</Position>';}
function waypoint(s){return '<Waypoint routeStrategy="shortest">'+position(lanePosition(s))+'</Waypoint>';}
function speed(value,duration) {
 return '<PrivateAction><LongitudinalAction><SpeedAction>'
 +'<SpeedActionDynamics dynamicsShape="'+(duration?'linear':'step')+'" value="'+(duration||0)+'" dynamicsDimension="time"/>'
 +'<SpeedActionTarget><AbsoluteTargetSpeed value="'+value+'"/></SpeedActionTarget>'
 +'</SpeedAction></LongitudinalAction></PrivateAction>';
}
function carInit(car,length){
 return '<Private entityRef="'+xml(car.name)+'">'
 +'<PrivateAction><TeleportAction>'+position(lanePosition(car.s))+'</TeleportAction></PrivateAction>'
 +speed(car.speed,0)
 +'<PrivateAction><RoutingAction><AssignRouteAction><Route name="PrawkoRoute_'+xml(car.name)+'" closed="false">'
 +waypoint(car.s)+waypoint(length-8)+'</Route></AssignRouteAction></RoutingAction></PrivateAction></Private>';
}
function timeCondition(key,seconds,rule){
 return '<Condition name="'+xml(key)+'" delay="0" conditionEdge="'+(rule==='greaterOrEqual'?'none':'rising')+'"><ByValueCondition>'
 +'<SimulationTimeCondition value="'+seconds+'" rule="'+(rule||'greaterThan')+'"/></ByValueCondition></Condition>';
}
function trigger(key,t){
 return '<StartTrigger><ConditionGroup>'+timeCondition(key,t)+'</ConditionGroup></StartTrigger>';
}
function braking(actor,name,t,target,duration){
 return {actor,xml:'<Event name="'+xml(name)+'" priority="override"><Action name="Akcja_'+xml(name)+'">'
 +speed(target,duration)+'</Action>'+trigger('Czas_'+name,t)+'</Event>'};
}
function walker(x,t){
 const pts=[[0,-6.8],[0.5,-5.8],[3.4,0],[6.0,4.6]]
 .map(([seconds,y])=>'<Vertex time="'+seconds+'">'+position(worldPosition(x,y))+'</Vertex>').join('');
 return {actor:'Pieszy',xml:'<Event name="Pieszy_wchodzi" priority="override"><Action name="Przejscie">'
 +'<PrivateAction><RoutingAction><FollowTrajectoryAction>'
 +'<TrajectoryRef><Trajectory name="Prawko_Zebra" closed="false"><Shape><Polyline>'+pts+'</Polyline></Shape></Trajectory></TrajectoryRef>'
 +'<TimeReference><Timing domainAbsoluteRelative="relative" scale="1" offset="0"/></TimeReference>'
 +'<TrajectoryFollowingMode followingMode="position"/></FollowTrajectoryAction></RoutingAction></PrivateAction>'
 +'</Action>'+trigger('Czas_wejscia',t)+'</Event>'};
}
function story(events){
 if(!events.length)return '';
 const groups=events.map((e,i)=>'<ManeuverGroup name="Grupa_'+(i+1)+'" maximumExecutionCount="1">'
 +'<Actors selectTriggeringEntities="false"><EntityRef entityRef="'+xml(e.actor)+'"/></Actors>'
 +'<Maneuver name="Manewr_'+(i+1)+'">'+e.xml+'</Maneuver></ManeuverGroup>').join('');
 return '<Story name="Trening_Prawko"><Act name="Scenariusz">'+groups+
 '<StartTrigger><ConditionGroup>'+timeCondition('Start',0,'greaterOrEqual')+'</ConditionGroup></StartTrigger>'
 +'</Act></Story>';
}
function scenario(s){
 const items=s.cars.map(c=>vehicle(c.name)).join('')+(s.zebra?pedestrian():'');
 const init=s.cars.map(c=>carInit(c,s.length)).join('')+
 (s.zebra?'<Private entityRef="Pieszy"><PrivateAction><TeleportAction>'
 +position(worldPosition(s.zebra,-6.8))+'</TeleportAction></PrivateAction></Private>':'');
 return ['<?xml version="1.0" encoding="UTF-8"?>',
 '<!-- Original authored OpenSCENARIO for PrawkoNaRaz, Apache-2.0 -->',
 '<OpenSCENARIO><FileHeader revMajor="1" revMinor="2" date="2026-11-15T00:00:00" description="'+xml(s.title)+'" author="PrawkoNaRaz"/>',
 '<ParameterDeclarations/><CatalogLocations/>',
 '<RoadNetwork><LogicFile filepath="'+s.name+'.xodr"/></RoadNetwork>',
 '<Entities>'+items+'</Entities>',
 '<Storyboard><Init><Actions>'+init+'</Actions></Init>',
 story(s.events),
 '<StopTrigger><ConditionGroup>'+timeCondition('Koniec',s.duration)+'</ConditionGroup></StopTrigger>',
 '</Storyboard></OpenSCENARIO>',''].join('\n');
}
const cases=[
 {name:'01-miasto-lusterka',title:'Miasto: ruszanie i obserwacja lusterek',length:240,speedLimit:50,duration:13,
 cars:[{name:'Ego',s:30,speed:10},{name:'AutoZa',s:6,speed:8},{name:'AutoPrzed',s:93,speed:7}],events:[],
 time:'day',weather:'clear',season:'summer',view:[70,0,1.2]},
 {name:'02-przejscie-pieszy',title:'Przejscie: pieszy i hamowanie',length:200,speedLimit:30,duration:11,zebra:86,
 cars:[{name:'Ego',s:15,speed:10},{name:'AutoZa',s:2,speed:6}],
 events:[braking('Ego','Hamowanie_przed_zebra',4.1,0,2.8),walker(86,4.0),braking('AutoZa','Bezpieczne_hamowanie_za_ego',5,0,2.7)],
 time:'dusk',weather:'clear',season:'autumn',view:[85,0,1.2]},
 {name:'03-deszcz-luk',title:'Deszcz: zakret i hamowanie pojazdu z przodu',length:240,speedLimit:50,duration:13,curve:true,
 cars:[{name:'Ego',s:25,speed:12},{name:'AutoPrzed',s:100,speed:8},{name:'AutoZa',s:3,speed:9}],
 events:[braking('AutoPrzed','Nagly_postoj',6,0,2.2),braking('Ego','Reakcja_kierowcy',7,0,3)],
 time:'dusk',weather:'rain',season:'autumn',view:[90,0,1.2]},
 {name:'04-zima-mgla',title:'Zima: ograniczona widocznosc i zwolnienie',length:210,speedLimit:40,duration:12,rural:true,
 cars:[{name:'Ego',s:18,speed:7.5},{name:'AutoPrzed',s:92,speed:5},{name:'AutoZa',s:2,speed:5.8}],
 events:[braking('Ego','Wolniej_we_mgle',7,4,2)],
 time:'day',weather:'fog',season:'winter',view:[80,0,1.2]}
];
for(const s of cases){
 fs.writeFileSync(path.join(folder,s.name+'.xodr'),road(s));
 fs.writeFileSync(path.join(folder,s.name+'.xosc'),scenario(s));
 const state={scene_version:1,mode:'scenario',textured:true,time_of_day:s.time,weather:s.weather,season:s.season,
 driver_view_actor:'Ego',
 view:{projection:'perspective',target:s.view,distance:43,pitch:0.73,yaw:-1.57}};
 fs.writeFileSync(path.join(folder,s.name+'.rmscene.json'),JSON.stringify(state,null,2)+'\n');
 console.log('NEW_PRAWKO_SCENE_CREATED',s.name,s.cars.length,'vehicles');
}
fs.writeFileSync(path.join(folder,'project.json'),JSON.stringify({project_version:2,name:'PrawkoNaRaz | Autorskie scenariusze',last_scene:'01-miasto-lusterka.xodr'},null,2)+'\n');
fs.writeFileSync(path.join(folder,'SCENARIUSZE.md'),[
'# Autorskie sceny PrawkoNaRaz',
'',
'Pliki OpenDRIVE i OpenSCENARIO przygotowane 15 listopada 2026 r. od podstaw (Apache-2.0).',
'',
'| Scena | Warunki | Zdarzenie |',
'| --- | --- | --- |',
'| 01-miasto-lusterka | dzien/lato | Samochod z przodu i z tylu, poruszajacy sie Ego |',
'| 02-przejscie-pieszy | zmierzch/jesien | Pieszy wchodzi na pasy, Ego hamuje |',
'| 03-deszcz-luk | deszcz/jesien | Luk drogi, AutoPrzed hamuje, Ego reaguje |',
'| 04-zima-mgla | mgla/zima | Wolniejsza jazda i ograniczona widocznosc |',
'',
'Otworz plik .xodr w edytorze, a w panelu Symulacja uruchom przejazd.',
'Odpowiadajace mu pliki .xosc i .rmscene.json zostana automatycznie odczytane.',
'Weryfikacja odczytu plikow nie oznacza automatycznie potwierdzenia przejazdu esmini.',
''
].join('\n'));
console.log('FOUR_AUTHOR_SCENES_READY',folder);
