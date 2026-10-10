#!/usr/bin/env node
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const html=fs.readFileSync(require('path').join(__dirname,'../web/jura-web-console.html'),'utf8');
const start=html.indexOf('function parseEepromPage('),end=html.indexOf('function saveState(');
assert(start!==-1 && end>start);
const parser=vm.runInNewContext(html.slice(start,end)+'\nparseDumpLog;');
let log=[];
for(let p=0;p<16;p++){
  const a=(p*16).toString(16).toUpperCase().padStart(2,'0');
  let ee='',ram='';
  for(let j=0;j<16;j++){ee+=(p*16+j).toString(16).toUpperCase().padStart(4,'0');ram+=(p*16+j).toString(16).toUpperCase().padStart(2,'0')}
  log.push(`EEPROM RT:${a} => rt:${ee}`);log.push(`RAM RR:${a} => rr:${ram}`);
}
const result=parser(log.join('\n'));
assert.equal(result.eepromPages.size,16);assert.equal(result.ramPages.size,16);
assert.equal(result.eepromMapped.size,256);assert.equal(result.ramMapped.size,256);
assert.equal(result.eepromMapped.get(0xFF),255);
assert.equal(result.ramMapped.get(0xFF),255);
console.log('Jura browser parser: 16 EEPROM + 16 RAM pages / 512 + 256 byte cells OK');
