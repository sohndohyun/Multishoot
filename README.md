# MultiShoot
SDL2를 기반으로 구현한 간단한 인베이더 같은 슈팅 게임입니다.
서버를 이용하여 여럿이서 플레이도 가능합니다.

## 싱글모드
MultishootClient 폴더 내의 VS프로젝트를 빌드하면 아래와 같은 화면이 나옵니다.

![image1](./readmeResource/image_1.png)

첫 화면에서 싱글모드를 선택하면 계속해서 내려오는 적을 상대합니다.

마지막 점수와 최대 점수를 xml로 저장합니다.

## 멀티모드

MultiShoot_server 를 빌드하여 실행한 후 멀티 모드로 접속하면 위 사진과 같이 여러대의 비행기가 동시에 같은 게임을 하게 됩니다.

![image2](./readmeResource/image_2.png)

위와 같이 여러 클라이언트를 띄워 동시에 플레이가 가능합니다.

![image3](./readmeResource/image_3.png)

많은 클라이언트를 동시에 띄우는 것도 가능합니다.
