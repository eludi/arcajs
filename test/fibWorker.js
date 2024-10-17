function fib(n) {
    return n<2 ? n : fib(n-2)+fib(n-1);
}

onmessage = function(msg) {
    console.log('onmessage(', msg, ')')
    if(msg.call=='fib') {
        postMessage("fibonacci("+msg.n+") calculation started");
        console.log('calculating fib('+msg.n+')...')
        const result = fib(msg.n);
        console.log(result)
        msg.result = result;
        postMessage(msg);
    }
}
console.log('fib(32):', fib(32));
if(typeof importScripts !== 'function')
    app.close()

