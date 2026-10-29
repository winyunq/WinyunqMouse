const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const assert = require('node:assert/strict');
class Element {
  constructor(tag) {this.tag=tag;this.value='';this.checked=false;this.children=[];this.listeners={};this.disabled=false;this.textContent='';this.classes=new Set();this.classList={toggle:(c,on)=>on?this.classes.add(c):this.classes.delete(c)};}
  appendChild(e) {this.children.push(e);return e;}
  replaceChildren(...nodes) {this.children=nodes;this.textContent='';}
  addEventListener(name, fn) {this.listeners[name]=fn;}
  click() {this.clicked=true;this.listeners.click?.();}
  remove() {this.removed=true;}
}
const elements=Object.fromEntries(['config-form','ncs','sclk','sdio','preview','status','download','swap-xy','invert-x','invert-y'].map(x=>[x,new Element(x)]));
const body=new Element('body');
const timers=[];const downloads=[];const revoked=[];
const ctx={document:{getElementById:id=>elements[id],createElement:tag=>new Element(tag),body},Blob,URL:{createObjectURL:blob=>{downloads.push(blob);return 'blob:test/'+downloads.length;},revokeObjectURL:url=>revoked.push(url)},setTimeout:fn=>timers.push(fn)};
const html = fs.readFileSync(path.join(__dirname, '../config/index.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
vm.createContext(ctx);vm.runInContext(script,ctx);
let count=0;const ok=(f)=>{f();count++;};
const good={NCS:'PA4',SCLK:'PA5',SDIO:'PB7'};
ok(()=>assert.equal(elements.download.disabled,true));
ok(()=>assert.equal(ctx.validateConfiguration({}).errors.length,3));
for(const pin of ['PA16','PB24','PA00','PA-1','PC1','<script>','PA1\n',0]) ok(()=>assert.equal(ctx.validateConfiguration({...good,NCS:pin}).valid,false));
for(const pin of ['PA9','PA12','PA13','PB22']) ok(()=>assert.equal(ctx.validateConfiguration({...good,NCS:pin}).valid,false));
ok(()=>assert.equal(ctx.validateConfiguration({...good,SCLK:'PA4'}).valid,false));
ok(()=>assert.equal(ctx.validateConfiguration(good).valid,true));
ok(()=>assert.throws(()=>ctx.generateHeader({NCS:'PA1',SCLK:'PA1',SDIO:'PB1'},{})));
for(let mask=0;mask<8;mask++) {
 const header=ctx.generateHeader(good,{swapXY:!!(mask&1),invertX:!!(mask&2),invertY:!!(mask&4)});
 ok(()=>{assert.match(header,/#define PAW3805EK_CONFIGURED 1/);assert.match(header,/#define PAW3805EK_NCS_PORT 0/);assert.match(header,/#define PAW3805EK_NCS_PIN 4/);assert.match(header,/#define PAW3805EK_SDIO_PORT 1/);assert.match(header,/#define PAW3805EK_SDIO_PIN 7/);assert.ok(header.includes('#define PAW3805EK_SWAP_XY '+((mask&1)?1:0)));assert.ok(header.includes('#define PAW3805EK_INVERT_X '+((mask&2)?1:0)));assert.ok(header.includes('#define PAW3805EK_INVERT_Y '+((mask&4)?1:0)));assert.ok(header.endsWith('\n'));});
}
ok(()=>{elements.download.listeners.click();assert.equal(downloads.length,0);});
elements.ncs.value='PA4';elements.sclk.value='PA5';elements.sdio.value='PB7';elements['swap-xy'].checked=true;
ctx.refresh();
ok(()=>{assert.equal(elements.download.disabled,false);assert.ok(elements.preview.textContent.includes('#define PAW3805EK_SWAP_XY 1'));});
ok(()=>{elements.download.click();elements.download.click();assert.equal(downloads.length,2);assert.equal(body.children.at(-1).download,'PAW3805EKConfig.h');assert.equal(body.children.at(-1).removed,true);timers.splice(0).forEach(fn=>fn());assert.equal(revoked.length,2);});
elements.sdio.value='PA4';ctx.refresh();
ok(()=>{assert.equal(elements.download.disabled,true);assert.ok(!elements.preview.textContent.includes('PAW3805EK_CONFIGURED 1'));elements.download.listeners.click();assert.equal(downloads.length,2);});
elements.ncs.value=elements.sclk.value=elements.sdio.value='';elements['swap-xy'].checked=false;elements['config-form'].listeners.reset();timers.splice(0).forEach(fn=>fn());
ok(()=>assert.equal(elements.download.disabled,true));
ok(()=>{let prevented=false;elements['config-form'].listeners.submit({preventDefault:()=>prevented=true});assert.equal(prevented,true);});
ok(()=>{for(const select of [elements.ncs,elements.sclk,elements.sdio]){assert.equal(select.children.length,2);assert.equal(select.children[0].children.length,16);assert.equal(select.children[1].children.length,24);for(const group of select.children)for(const opt of group.children)if(['PA9','PA12','PA13','PB22'].includes(opt.value)) assert.equal(opt.disabled,true);}});
console.log('PASS '+count+' assertions: validation, orientation, preview, repeated download, revocation, reset, reserved options.');
