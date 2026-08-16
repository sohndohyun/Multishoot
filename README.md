# Multishoot

Windows와 SDL2로 만든 2D 슈팅 게임입니다. 로컬 싱글 플레이와 IOCP TCP 서버 기반 멀티플레이가 동일한 `MultishootCommon` 게임 시뮬레이션을 사용합니다.

## 프로젝트 구조

루트의 `Multishoot.sln`에는 다음 프로젝트가 포함됩니다.

| 프로젝트 | 역할 |
| --- | --- |
| `DRLib` | 프레임, 버퍼, 자료구조, IOCP 클라이언트·서버, tinyxml2 공용 정적 라이브러리 |
| `MultishootCommon` | 게임 규칙·상태·이벤트와 Protobuf 생성 코드를 소유하는 정적 라이브러리 |
| `Multishoot` | SDL2 렌더링, 입력, 싱글·멀티플레이 클라이언트 어댑터 |
| `MultishootServer` | 소켓 연결, 전용 DB 워커, 공통 시뮬레이션 이벤트 전송을 담당하는 서버 어댑터 |
| `MultishootCommonTests` | 외부 테스트 프레임워크 없이 공통 시뮬레이션과 프로토콜을 검사하는 콘솔 테스트 |

```text
.
├─ Multishoot.sln
├─ vcpkg.json
├─ docker-compose.yml
├─ docker/mysql/init/001_schema.sql
├─ DRLib/
├─ MultishootCommon/
│  ├─ game/
│  └─ generated/cpp/
├─ Multishoot/
│  ├─ engine/
│  ├─ entities/
│  ├─ controllers/
│  ├─ scenes/
│  └─ resource/
├─ MultishootServer/
│  ├─ database/
│  └─ game/
├─ schema/
│  ├─ schema/multishoot/protocol/game.proto
│  └─ parse.bat
└─ tests/
```

클라이언트와 서버는 `MultishootCommon`과 `DRLib`을 참조합니다. 서버의 MySQL 연결과 인증 연산은 전용 워커 스레드 하나에서 실행되고, 네트워크 콜백은 결과 큐만 처리합니다.

## 빌드 준비

- Windows 10 SDK
- MSVC platform toolset `v145`
- C++ `stdcpplatest`
- Visual Studio 18 이상과 C++용 vcpkg 구성 요소
- Docker Desktop (Compose 포함)

루트 `vcpkg.json`은 `libmysql`, Protobuf `3.21.12`, SDL2/SDL2_ttf와 registry baseline을 고정합니다. 각 C++ 프로젝트는 manifest mode를 사용하며 플랫폼에 따라 `x64-windows` 또는 `x86-windows` triplet을 선택합니다. Visual Studio/MSBuild가 첫 빌드에서 필요한 패키지를 자동 복원합니다.

번들 `protoc.exe`도 `3.21.12`입니다. 생성된 다음 파일은 `MultishootCommon`의 빌드 입력이자 Git 관리 대상입니다.

```text
MultishootCommon/generated/cpp/multishoot/protocol/game.pb.h
MultishootCommon/generated/cpp/multishoot/protocol/game.pb.cc
```

`game.proto`를 변경한 경우 생성 파일을 수동으로 갱신해 스키마와 함께 커밋합니다.

```powershell
& '.\schema\parse.bat'
```

## 빌드

```powershell
.\build.bat
.\build.bat Release
```

`build.bat`는 x64 Debug를 기본으로 빌드하며 인자로 Release를 지정할 수 있습니다. 실행 파일은 예를 들어 `build\x64\Debug\MultishootServer`처럼 프로젝트별 출력 폴더에 생성됩니다.

## 실행

Docker MySQL과 서버를 순서대로 실행합니다.

```powershell
.\run.bat db-up
.\run.bat server
```

`db-up`은 MySQL healthcheck를 통과할 때까지 기다립니다. 데이터는 `multishoot_mysql_data` 볼륨에 보존됩니다. 컨테이너만 중지하려면 `db-down`을 사용하고, 로컬 계정과 스키마를 모두 지우고 다시 만들 때만 `db-reset`을 사용합니다.

```powershell
.\run.bat db-down
.\run.bat db-reset
```

클라이언트는 리소스 상대 경로를 위해 프로젝트 폴더에서 실행합니다.

```powershell
.\run.bat client
```

필요한 실행 파일이 없으면 `run.bat`가 해당 구성의 `build.bat`를 자동으로 실행합니다.

서버 기본값은 다음과 같습니다. 기존 단일 포트 인자도 호환되며 명명 옵션으로 덮어쓸 수 있습니다.

```text
--port 3000 --db-host 127.0.0.1 --db-port 3306
--db-user multishoot --db-password multishoot_dev --db-name multishoot
```

예: `run.bat server Debug --db-name multishoot_test`. Compose 자격 증명은 로컬 개발 전용 값이며 배포 환경에서 사용하지 마십시오.

## 조작법

| 화면 | 입력 | 동작 |
| --- | --- | --- |
| 로비 | 위/아래 방향키 | 싱글·멀티 모드 선택 |
| 로비 | Space | 선택한 모드 시작 |
| 인증 | 좌/우 방향키 | 로그인·회원가입 전환 |
| 인증 | Tab 또는 위/아래 방향키 | 입력 필드 이동 |
| 인증 | Enter / Backspace / Esc | 제출 / 한 글자 삭제 / 로비 복귀 |
| 게임 | 방향키 | 플레이어 이동 |
| 게임 | Space | 발사 |

멀티 계정은 MySQL에 저장됩니다. 아이디는 영문자·숫자·밑줄 3~16자, 비밀번호는 공백 없는 ASCII 8~64자입니다. 회원가입 성공 후 같은 TCP 연결로 즉시 게임에 입장하며, 멀티 HUD에는 계정별 최고 점수가 표시됩니다. 서버 재시작으로 계정이 초기화되지 않고 `db-reset`을 실행할 때만 초기화됩니다.

## 테스트

네트워크 테스트 전에 MySQL Compose 서비스를 실행해야 합니다.

```powershell
.\run.bat db-up
.\run.bat test
```

`run.bat test`는 `multishoot_test` DB만 초기화한 뒤 공통 테스트와 네트워크 스모크 테스트를 실행합니다. 인증 전 게임 차단, 가입·로그인·중복 접속, 서버 재시작 후 계정 유지, oversized/malformed/분할 프레임, 발사 쿨다운을 검사합니다.

Release 실행은 두 번째 인자로 구성을 지정합니다.

```powershell
.\run.bat server Release
.\run.bat client Release
.\run.bat test Release
```

전체 빌드 행렬은 다음처럼 확인할 수 있습니다.

```powershell
foreach ($configuration in 'Debug', 'Release') {
    foreach ($platform in 'x64', 'Win32') {
        msbuild .\Multishoot.sln /m /t:Build /p:Configuration=$configuration /p:Platform=$platform
    }
}
```

## 화면

![로비와 싱글 모드](./readmeResource/image_1.png)

![멀티플레이](./readmeResource/image_2.png)

![다수 클라이언트](./readmeResource/image_3.png)

## 프로토콜과 제약 사항

- `ClientPacket`과 `ServerPacket`의 Protobuf `oneof`가 유일한 게임 wire 타입입니다.
- 직렬화된 메시지 하나를 DRLib의 8바이트 헤더 뒤에 담으며 payload는 최대 504바이트입니다.
- 멀티플레이는 이벤트 기반 동기화이며 위치 스냅샷, 관전자, 자동 재접속은 제공하지 않습니다.
- 계정과 멀티 최고 점수는 Docker MySQL에 저장되며, 비밀번호는 무작위 16바이트 salt와 PBKDF2-HMAC-SHA256 600,000회 해시로만 보관합니다.
- Windows IOCP와 Winsock2를 사용하는 Windows 전용 프로젝트입니다.
- 게임 TCP와 DB 연결에 배포용 TLS 정책은 제공하지 않으며, 인터넷 배포용 인증 시스템이 아닙니다.
