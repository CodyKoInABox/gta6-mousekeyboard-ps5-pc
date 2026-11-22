# PS5 Keyboard Bridge: guia de configuração

Este guia leva você do download até o primeiro jogo com teclado e mouse no PS5.

O app envia só os comandos do controle para o PS5, por Remote Play, sem vídeo.
A imagem continua indo para a sua capturadora (Ripsaw) e para o Veyra NRVideo, como antes.

## 1. O que você precisa

- PC com Windows 10 ou Windows 11, 64 bits. O app foi testado no Windows 10 (build 19045).
- PS5 e PC na mesma rede (de preferência cabo ou Wi-Fi estável).
- A conta PSN que você usa no PS5, com a senha à mão (ela só é digitada no site da Sony).
- O Veyra NRVideo instalado, se você quer ver a imagem da capturadora no PC.

## 2. Instalar

1. Baixe `PS5KeyboardBridge-Setup-<versão>.exe` na página de releases do projeto.
2. Confira a integridade (opcional, recomendado). No PowerShell, na pasta do download:

   ```
   Get-FileHash .\PS5KeyboardBridge-Setup-<versão>.exe -Algorithm SHA256
   ```

   O resultado deve bater com a linha correspondente em `SHA256SUMS.txt`.
3. Execute o instalador. Ele não pede senha de administrador e instala só para o seu usuário.
4. Se o Windows mostrar "O Windows protegeu o computador", clique em **Mais informações** e depois em **Executar assim mesmo**. O aviso aparece porque o app ainda não tem assinatura digital.
5. Escolha se quer o atalho na área de trabalho e clique em **Concluir**.

Também existe um pacote portátil (`PS5KeyboardBridge-<versão>-portable.zip`). Extraia a pasta em qualquer lugar e rode `PS5KeyboardBridge.exe`. Ele funciona do mesmo jeito, sem instalar.

## 3. Preparar o PS5

Faça estes ajustes no PS5 uma vez:

1. **Configurações > Sistema > Remote Play**: marque **Ativar Remote Play**.
2. **Configurações > Sistema > Economia de energia > Recursos disponíveis em modo de repouso**:
   - marque **Manter conectado à Internet**;
   - marque **Ativar ligação do PS5 pela rede**.
3. Deixe o PS5 ligado ou em repouso, na mesma rede do PC.

Feche o PS Remote Play no PC e no celular antes de jogar. O PS5 aceita só uma conexão de Remote Play por vez.

## 4. Primeira configuração (assistente)

Na primeira vez, o app abre o assistente **Configuração inicial**. Você pode reabri-lo depois em **Configuração inicial** na tela principal.

1. **Boas-vindas**: o app confere se o Windows é compatível.
2. **Encontrar o PS5**: clique em **Procurar PS5**. Se não aparecer, digite o IP do PS5 e clique em **Usar este IP**. Você acha o IP do PS5 em **Configurações > Rede > Ver status da conexão** no próprio PS5.
3. **Remote Play**: confira os ajustes do passo 3 deste guia.
4. **Conta PSN**:
   1. clique em **Abrir login da Sony**. O navegador abre o site oficial da Sony;
   2. entre com a sua conta PSN (a senha vai só para o site da Sony);
   3. na página final ("redirect"), copie a URL inteira da barra de endereços e cole no campo do assistente;
   4. clique em **Concluir login**.
   Se você já sabe o ID da conta, pode digitá-lo diretamente.
5. **Código de pareamento**: no PS5, abra **Configurações > Sistema > Remote Play > Link Device**. A tela mostra um código de 8 dígitos que vale por alguns minutos. Digite o código e clique em **Parear com o PS5**.
6. **Teste de conexão**: o app conecta e envia um toque no botão X. Você deve ver o X ser pressionado no PS5. A imagem não é usada nesse teste.
7. **Veyra** (opcional): se o app não achar o Veyra.exe, escolha o arquivo. O app nunca altera a configuração do Veyra.
8. **Pronto para jogar**: clique em **Concluir**.

O pareamento fica salvo de forma criptografada, protegida pela sua conta do Windows. Você não precisa refazê-lo, a menos que troque o PS5 ou o PS5 esqueça o PC.

## 5. Jogar

1. Abra o Veyra, se quiser a imagem da capturadora.
2. Na tela principal do PS5 KB Bridge, clique em **JOGAR**.
3. Quando o Veyra estiver em foco, o teclado e o mouse controlam o jogo no PS5. F8 liga e desliga a captura.
4. Para soltar tudo na hora, pressione **Esc**. Para parar a conexão, clique em **PARAR**.

Enquanto a captura está ativa, o app bloqueia as teclas e o clique para o Veyra, para que um clique de jogo não mexa na janela dele. Alt+Tab, Win e Ctrl+Alt+Del continuam funcionando.

### Controles padrão (GTA 6)

| No PC | No PS5 |
|---|---|
| W A S D | Analógico esquerdo (andar) |
| Mouse | Analógico direito (mira/câmera), traduzido para o analógico |
| Clique esquerdo | R2 |
| Clique direito | L2 |
| Clique do meio | R3 |
| Espaço | X |
| Ctrl esquerdo | Círculo |
| E | Triângulo |
| R | Quadrado |
| Shift esquerdo | L3 |
| Q | L1 |
| F | R1 |
| Tab | Touchpad |
| Enter | Options |
| Setas | Direcional (D-pad) |
| Backspace | Share |
| F8 | Liga e desliga a captura |
| Esc | Solta tudo e desarma a captura |

O perfil **Dirigir (volante suave)** troca W/S por gatilhos e suaviza o volante com A/D.

### Sobre a mira com o mouse

O mouse não é um analógico. O app converte o movimento do mouse em movimento do analógico direito, com limites:

- O analógico tem alcance máximo. Movimentos rápidos e longos do mouse chegam ao limite e não viram mais movimento.
- Os jogos aplicam suas próprias curvas de mira e limites de velocidade de giro. Isso pode fazer a mira parecer mais lenta ou mais rápida que no PC.
- Ajuste a sensibilidade e a zona morta em **Configurações > Ajustes avançados de mira**. Mexer nelas só é necessário se a mira estiver fora do seu gosto.

## 6. Configurações

Abra **Configurações** na tela principal.

- **Perfil de controle**: GTA 6 (padrão) ou Dirigir.
- **Caminho do Veyra.exe**: deixe vazio para o app achar sozinho, ou escolha o arquivo.
- **Tecla que liga e desliga a captura**: F8 por padrão.
- **Conectar ao abrir o app**: entra no PS5 assim que o app abre.
- **Reconectar automaticamente**: tenta de novo se a conexão cair.
- **Minimizar para a bandeja**: fechar a janela deixa o app rodando perto do relógio.
- **Abrir o Veyra ao jogar**: abre o Veyra ou traz a janela dele para frente.
- **Ajustes avançados de mira**: sensibilidade, zona morta, anti-zona morta, aceleração e inverter o eixo Y. Deixe escondido se não quiser mexer.
- **Esquecer PS5**: apaga o pareamento salvo.

## 7. Onde ficam os dados

Seus dados ficam em `%LOCALAPPDATA%\PS5KeyboardBridge`: o pareamento (criptografado), as configurações e o perfil de controle.

Desinstalar o app mantém essa pasta, então você não precisa parear de novo ao reinstalar. Para apagar tudo, remova essa pasta depois de desinstalar.

## 8. Problemas

Veja `solucao-de-problemas.md`.
