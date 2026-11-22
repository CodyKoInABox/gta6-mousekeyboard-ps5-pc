# Resultados dos testes

Versão testada: 0.1.0, construída por `scripts/build-release.ps1` a partir do branch `cursor/installer-f066`.
Máquina: Windows 10 Pro 22H2 (build 19045), x64, Visual Studio 2022 (MSVC), CMake 3.23+.

## 1. Testes automatizados

Comando: `scripts\build-release.ps1` (roda `pskb_tests.exe` antes de empacotar).

- Resultado: **161 testes, 0 falhas**.
- Saída completa: `dist\test-results.txt`.
- Cobertura: mapeamento de teclas e gatilhos, mouse para analógico (zona morta, curva, aceleração, retorno ao centro), perfis, estado do link e reconexão (com backend simulado), captura e gate de entrada, pareamento e armazenamento DPAPI, configurações, normalização de PIN, status do app e etapas do assistente, detecção do Veyra.

Os testes não usam PS5 nem rede.

## 2. Build reproduzível

Dois builds completos seguidos a partir do mesmo commit produziram arquivos idênticos:

| Artefato | SHA-256 |
|---|---|
| `PS5KeyboardBridge-Setup-0.1.0.exe` | ver `dist\SHA256SUMS.txt` (bate entre builds) |
| `PS5KeyboardBridge-0.1.0-portable.zip` | ver `dist\SHA256SUMS.txt` (bate entre builds) |

O que torna o build reproduzível:

- `/Brepro` no linker, para que o timestamp do PE venha do conteúdo.
- Entradas do zip com timestamp fixo e ordem ordenada.
- Data de modificação dos arquivos do instalador fixada em 2026-01-01 (o Inno Setup grava essas datas; sem isso, cada cópia mudava o instalador).
- Diretório de build limpo a cada execução.

Durante a verificação, o instalador mudou entre builds por causa das datas de modificação dos arquivos copiados. Isso foi corrigido e os dois builds seguintes ficaram idênticos.

## 3. Instalador

Teste feito com instalação silenciosa em uma pasta temporária (não no perfil do usuário):

- Instalação concluiu com código 0. Os arquivos do app, o uninstaller e os guias foram copiados.
- Opção de atalho na área de trabalho respeitada (não criado quando desmarcada).
- Desinstalação silenciosa concluiu com código 0 e removeu os binários.
- A pasta de dados `%LOCALAPPDATA%\PS5KeyboardBridge` não foi criada pelo instalador nem pela desinstalação, como esperado.
- O instalador roda sem privilégios de administrador (`PrivilegesRequired=lowest`).

Não testado: instalação no perfil real do usuário, atualização por cima de uma versão anterior, e a tela gráfica do instalador.

## 4. Assistente de configuração (testado neste PC com PS5 real na rede)

Ocorreu com o PS5 ligado na mesma rede (encontrado como `PS5-586`, em `192.168.15.9`, status "pronto").

| Etapa | Resultado |
|---|---|
| Boas-vindas | "Windows compatível." em verde. |
| Encontrar o PS5 (**Procurar PS5**) | Encontrou o PS5 e listou o console. |
| Escolher o console e **Avançar** | Habilitou o avanço. |
| Ativar o Remote Play (instruções) | Texto correto, sem erros de layout. |
| Conta PSN: ID inválido (`abc`) | "ID inválido. Use o número ou o código de 12 caracteres." |
| Conta PSN: ID válido digitado | Reconhecido e habilitou o avanço. Nada foi salvo. |
| Código de PIN com 3 dígitos | "O código tem 8 dígitos. Exemplo: 1234 5678." Recusado antes de qualquer conexão. |
| Teste de conexão (visual) | Página renderizada. Não foi executado com pareamento real. |
| Veyra (**Procurar automaticamente**) | Encontrou `C:\Users\marua\AppData\Local\Programs\Veyra (NVIDIA)\Veyra.exe`. |
| Tela final "Pronto para jogar" | Renderizada antes da troca do título grande. Esse ajuste ainda não foi reexibido na tela. |

Correções feitas nesta etapa:

- A tela final repetia "Pronto para jogar" no título e no texto grande. Agora o texto grande é "Configuração concluída.".
- Fechar a janela do assistente durante uma tarefa (por exemplo, o teste de conexão, que pode levar até 40 s) travava a interface. Agora mostra "Aguarde a tarefa terminar antes de fechar." e só fecha quando a tarefa termina.

Não testado nesta etapa: a sessão de login da Sony (exige a sua conta), o pareamento real com o PIN, e o teste de conexão com o PS5 pareado. Esses passos dependem de você e estão no checklist de hardware.

## 5. Não verificado ainda

- Pareamento real com o PIN e o teste de conexão no PS5.
- Sessão de jogo: captura, entrada de teclado e mouse, bloqueio no Veyra, Esc e F8 com o jogo rodando.
- Reconexão após queda de rede e tela de conflito com outros clientes de Remote Play.
- Firewall do Windows na primeira busca de rede.
- Windows 10 em outras builds além de 19045, e Windows 11.

Esses itens estão em `docs/checklist-hardware.md`.
