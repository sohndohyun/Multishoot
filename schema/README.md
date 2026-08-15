# Multishoot Schema

Multishoot 클라이언트와 서버가 함께 사용하는 Protocol Buffer 스키마입니다. 저장소에 포함된 `protoc 3.21.12`로 공유 C++ 코드를 생성하고, 같은 버전의 C++ 런타임은 루트 vcpkg manifest가 복원합니다.

## 디렉토리 구조

```text
schema/
├─ bin/
│  ├─ protoc.exe
│  ├─ libprotoc.dll
│  └─ libprotobuf.dll
├─ schema/multishoot/protocol/game.proto
├─ buf.yaml
├─ buf.gen.yaml
└─ parse.bat
```

생성 코드는 `../MultishootCommon/generated/cpp/`에 저장됩니다.

`schema/schema/`가 protobuf import 기준 경로입니다. import는 이 경로를 기준으로 작성합니다.

```proto
import "multishoot/protocol/game.proto";
```

## 코드 생성

빌드 중에는 `parse.bat`가 자동 실행되지 않습니다. `game.proto`를 변경했을 때 수동으로 실행합니다.

```powershell
& '.\schema\parse.bat'
```

생성 결과는 다음 위치에 기록되며 `MultishootCommon` 빌드 입력과 Git 관리 대상에 포함됩니다.

```text
../MultishootCommon/generated/cpp/multishoot/protocol/game.pb.h
../MultishootCommon/generated/cpp/multishoot/protocol/game.pb.cc
```

`parse.bat`는 Buf CLI 없이 `schema/` 아래의 모든 `.proto` 파일을 재귀적으로 컴파일합니다. 실행 후 변경된 생성 파일을 스키마와 함께 커밋합니다. 생성 코드와 런타임의 버전을 맞춰야 하므로 번들 compiler와 `vcpkg.json`의 Protobuf 버전을 함께 변경해야 합니다.

## 스키마 작성 규칙

- 파일명과 필드명은 `lower_snake_case`를 사용합니다.
- 메시지 타입은 `UpperCamelCase`를 사용합니다.
- 필드 번호는 배포 후 변경하거나 재사용하지 않습니다.
- 클라이언트 요청은 `ClientPacket`, 서버 응답은 `ServerPacket`의 `oneof`에 추가합니다.
- 클라이언트와 서버는 프로토콜 변경 시 함께 빌드하고 배포합니다.

## 선택적 Buf 명령

Buf CLI가 설치되어 있다면 스키마를 추가로 검사할 수 있습니다.

```powershell
buf lint
buf build
```

기본 코드 생성 경로는 `parse.bat`이며 `buf.gen.yaml`도 같은 `MultishootCommon/generated/cpp` 경로를 사용합니다.
