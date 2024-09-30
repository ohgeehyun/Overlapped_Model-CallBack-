Overlapped 모델을 사용한 asynchronous nonblocking socket

콜백 함수를 사용하여 비동기식으로 일처리가 끝난 이후 등록해둔 콜백함수를 실행시켜 가르처준다.

callback함수 와 WSAOverlapped를 사용한 소켓

비동기식 논 블로킹 소켓 이라는 특징을 가지고 있다.

장점 - 확실히 처음 아무 모델도 사용하지않은 논블로킹 소켓보다는 코드가 깔끔해젔다.
       select , WSAEventslect, 보다 뛰어난 성능

단점 - 모든 비동기 소켓 함수에서 사용가능하지 않음(accept)
번번한 alertable wait으로 인한 성능저하있음. alertable wait으로 결국 해당 스레드는 apc큐에 쌓인 일을 처리하고 나오는 것

주의점
스레드가 가지고있는 apc 큐에 진입을하여 일을 처리한다. 이것이 양날의 검이 될 수 있다.
또한 alertable wait 상태로 들어가 apc 큐에 진입을하여 일을 처리하는데 문제는 alertable wait상태를 다른 놀고있는 스레드나 다른 스레드에게 줄 수는 없다.
