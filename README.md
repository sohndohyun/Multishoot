# Multishoot

Windows와 SDL2로 만든 2D 슈팅 게임입니다. 로컬 싱글 플레이와 IOCP TCP 서버 기반 멀티플레이가 동일한 `MultishootCommon` 게임 시뮬레이션을 사용합니다.

## 프로젝트 구조

루트의 `Multishoot.sln`에는 다음 프로젝트가 포함됩니다.

| 프로젝트 | 역할 |
| --- | --- |
| `DRLib` | 프레임, 버퍼, 자료구조, IOCP 클라이언트·서버, tinyxml2 공용 정적 라이브러리 |
| `MultishootCommon` | 게임 규칙·상태·이벤트와 Protobuf 생성 코드를 소유하는 정적 라이브러리 |
| `Multishoot` | SDL2 렌더링, 입력, 싱글·멀티플레이 클라이언트 어댑터 |
| `MultishootServer` | 소켓 연결과 공통 시뮬레이션 이벤트 전송을 담당하는 서버 어댑터 |
| `MultishootCommonTests` | 외부 테스트 프레임워크 없이 공통 시뮬레이션과 프로토콜을 검사하는 콘솔 테스트 |

```text
.
├─ Multishoot.sln
├─ vcpkg.json
├─ DRLib/
├─ MultishootCommon/
│  └─ game/
├─ Multishoot/
│  ├─ engine/
│  ├─ entities/
│  ├─ controllers/
│  ├─ scenes/
│  └─ resource/
├─ MultishootServer/
│  └─ game/
├─ schema/
│  ├─ schema/multishoot/protocol/game.proto
│  ├─ generated/cpp/
│  └─ parse.bat
└─ tests/
```

클라이언트와 서버는 `MultishootCommon`과 `DRLib`을 참조합니다. 싱글 모드는 공통 시뮬레이션 이벤트를 SDL 씬 큐로 전달하고, 서버는 같은 이벤트를 대상 소켓 또는 활성 소켓 전체에 전송합니다.

## 빌드 준비

- Windows 10 SDK
- MSVC platform toolset `v145`
- C++ `stdcpplatest`
- Visual Studio 18 이상과 C++용 vcpkg 구성 요소

루트 `vcpkg.json`은 Protobuf `3.21.12`와 registry baseline을 고정하고 SDL2/SDL2_ttf도 복원합니다. 각 C++ 프로젝트는 manifest mode를 사용하며 플랫폼에 따라 `x64-windows` 또는 `x86-windows` triplet을 선택합니다. Visual Studio/MSBuild가 첫 빌드에서 필요한 패키지를 자동 복원합니다.

번들 `protoc.exe`도 `3.21.12`입니다. `game.proto`가 바뀌면 `MultishootCommon`의 빌드 전 단계에서 `schema\parse.bat`가 실행되며 다음 파일을 다시 만듭니다.

```text
schema/generated/cpp/multishoot/protocol/game.pb.h
schema/generated/cpp/multishoot/protocol/game.pb.cc
```

생성 파일과 `vcpkg_installed/`는 Git에 포함하지 않습니다. 필요하면 수동으로도 생성할 수 있습니다.

```powershell
& '.\schema\parse.bat'
```

## 빌드

Visual Studio에서 `Multishoot.sln`을 열거나 Developer PowerShell에서 빌드합니다.

```powershell
msbuild .\Multishoot.sln /m /t:Build /p:Configuration=Debug /p:Platform=x64
```

지원 구성은 Debug/Release와 x64/Win32입니다. 출력은 예를 들어 `x64\Debug`에 생성됩니다. 모든 프로젝트는 Debug에서 `/MDd`, Release에서 `/MD`를 사용합니다.

## 실행

멀티플레이는 서버를 먼저 실행합니다.

```powershell
& '.\x64\Debug\MultishootServer.exe'
```

클라이언트는 리소스 상대 경로를 위해 프로젝트 폴더에서 실행합니다.

```powershell
Push-Location '.\Multishoot'
& '..\x64\Debug\Multishoot.exe'
Pop-Location
```

서버와 클라이언트는 기본적으로 `127.0.0.1:3000`을 사용합니다. 프로토콜은 호환성이 깨진 상태로 함께 변경되므로 클라이언트와 서버를 항상 같은 빌드로 배포해야 합니다.

## 조작법

| 화면 | 입력 | 동작 |
| --- | --- | --- |
| 로비 | 위/아래 방향키 | 싱글·멀티 모드 선택 |
| 로비 | Space | 선택한 모드 시작 |
| 게임 | 방향키 | 플레이어 이동 |
| 게임 | Space | 발사 |

## 테스트

공통 규칙·충돌·점수·세션 초기화 및 모든 Protobuf 메시지의 round-trip/크기를 검사합니다.

```powershell
& '.\x64\Debug\MultishootCommonTests.exe'
```

네트워크 스모크 테스트는 실제 서버를 실행해 oversized frame 연결 종료, malformed/empty Protobuf 무시, 분할 프레임 재조립, 서버 권위형 발사 쿨다운을 검사합니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tests\NetworkSmoke.ps1
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
- Windows IOCP와 Winsock2를 사용하는 Windows 전용 프로젝트입니다.
- 암호화와 인증은 지원하지 않습니다.
