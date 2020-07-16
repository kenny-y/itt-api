const itt = require('./build/Release/itt');
const Assert = require('assert');

Assert(typeof itt.create_domain === 'function');
Assert(typeof itt.create_string_handle === 'function');
Assert(typeof itt.task_begin === 'function');
Assert(typeof itt.task_end === 'function');

// console.log(itt);

const domain = itt.create_domain('my-domain');
console.log(domain);

const task = itt.create_string_handle('my-task');
console.log(task);

itt.task_begin(domain, task);
// Do something
itt.task_end(domain);

console.log('done');
