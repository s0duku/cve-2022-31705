
# CVE-2022-31705

## Intro

* https://www.vmware.com/content/vmware/vmware-published-sites/us/security/advisories/VMSA-2022-0033.html.html
* Test on windows vmware workstation 16.2.0, with guest os ubuntu server 22
* At leat one usb device is be attached to guest machine
* change the 'qh->field4' in the ehci.c code
* Type 'sudo ./run.sh' to compile and run the poc

## Example

* use 'lsusb' to determine the device id, use the device id to set 'qh->field4'  

![](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/13d5dfa5-3376-414e-a89c-819e3b80f0a8/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20230109%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20230109T074847Z&X-Amz-Expires=86400&X-Amz-Signature=6140baba04353256cc66500949d17dbf012347e68323574de2670dcfd4682651&X-Amz-SignedHeaders=host&response-content-disposition=filename%3D%22Untitled.png%22&x-id=GetObject)  

* vmx code will alloc a struct with 0x18 size (the size can be controlled in the code) buffer for reading guest physical memory.  

![](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/a4486050-9bf2-45da-944d-110847734526/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20230109%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20230109T075113Z&X-Amz-Expires=86400&X-Amz-Signature=bd9b4126e49b1a5d6f8bdb9e703ae96c471f996843245ab28369503c1f7aefd3&X-Amz-SignedHeaders=host&response-content-disposition=filename%3D%22Untitled.png%22&x-id=GetObject)  

* the total size of this struct will be 0x400  

![](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/ce3a8ebf-95e8-4918-8e3f-d6690f8561c3/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20230109%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20230109T075336Z&X-Amz-Expires=86400&X-Amz-Signature=59e25c5fdce1e2bc74d8847f2f6a0001faa185f550c96ac06a38d8d2d3c4e6a8&X-Amz-SignedHeaders=host&response-content-disposition=filename%3D%22Untitled.png%22&x-id=GetObject)  

* address of this struct is 0x26d6caa80d0  

![](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/a8e6ff74-a8e9-480e-b1fe-6e3681b1697b/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20230109%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20230109T075523Z&X-Amz-Expires=86400&X-Amz-Signature=b8509cadf8a3eb9c95855c10a30a94bd53b162c17ec8453fe0624a189671f6a5&X-Amz-SignedHeaders=host&response-content-disposition=filename%3D%22Untitled.png%22&x-id=GetObject)  

* in the end, vmx will read 0x800 size (can be controlled in the code) of physical memory into 0x2606CAA84C0, which is the last 0x10 byte of 0x26d6caa80d0 (first 0x8 is already readed at the alloc stage)  

![](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/2ba45b77-d246-48df-b877-97bfb780d78e/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20230109%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20230109T075805Z&X-Amz-Expires=86400&X-Amz-Signature=90107fb78f934b9082f520b10b4fd96df27595ca126a21bffbb09b21c8114e75&X-Amz-SignedHeaders=host&response-content-disposition=filename%3D%22Untitled.png%22&x-id=GetObject)  

* for sure, it will cause a OOB write.  

![](https://s3.us-west-2.amazonaws.com/secure.notion-static.com/2211aea8-5026-425d-bad0-5a263f6705c8/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Content-Sha256=UNSIGNED-PAYLOAD&X-Amz-Credential=AKIAT73L2G45EIPT3X45%2F20230109%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20230109T080241Z&X-Amz-Expires=86400&X-Amz-Signature=e2e9d67d5dc4ee3b45e64ea70253345d9098753f37f5c3cd71ce16487aeb44c8&X-Amz-SignedHeaders=host&response-content-disposition=filename%3D%22Untitled.png%22&x-id=GetObject)  




