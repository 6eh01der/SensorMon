<h1 align=center>Устройство отслеживания состояния датчиков с функцией SMS уведомлений(работа с датчиками протестирована, нужно больше тестов в отношении модема)</h1>
<p>
   &nbsp;Программа прошивается на контроллер из среды Platformio (возможно и из Arduino IDE).
</p>
<h2 align=center>Описание работы устройства</h2>
<p>
  &nbsp;Устройство предназначено для отслеживания состояния простых датчиков (открыто/закрыто, по сути кнопок). Благодаря использованию режима выводов INPUT_PULLUP для подключения не требуется резистор, и достаточно подключить первый и второй датчик к выводам 11 и 12 соответственно и к выводам заземления (при необходимости достаточно просто можно обеспечить работу с большим количеством датчиков).
</p>
<p>&nbsp;
  После установки устройства производится фиксирование номера телефона администратора и номера телефона дублера, после чего с номера телефона администратора соответствующими командами выставляются параметры работы устройства.
  </p>
  <h3 align=center>Установки</h3>
<p>&nbsp;
  <b>Установка номера администратора:</b> звонок на номер карточки установленной в прибор, прибор отреагирует звуковой и светодиодной сигнализацией, нажать на кнопку на плате прибора до отбоя звонка. Прибор ответит длинным звуковым сигналом и оповестит по смс о принятии номера админа.
</p>
<p>&nbsp;
  <b>Установка номера дублера:</b> отправить с номера админа смс с номером телефона дублера в формате «+7**********». Прибор оповестит по смс о принятии номера дублера. Дублёр может быть один. Для удаления номера отправить СМС с номером дублера на устройство повторно.
</p>
&nbsp;<b>Команды смс для настройки параметров:</b>
<ul>
  <li><b>«On 00»</b> - Выставляется время задержки включения режима отслеживания. Допустимые значения от 00 до 99 (секунды). Пример: смс с текстом «On 25» установит задержку включения отслеживания на 25 секунд.
  <li><b>«Of 00»</b> - Выставляется время задержки отключения отслеживания.
  <li><b>«Bl 00»</b> - Выставляется порог баланса средств на счету для оповещения. При понижении баланса ниже установленного, устройство оповестит об этом по смс один раз в сутки. Допустимые значения от 00 до 99.
  <li><b>«*000#»</b> - Выставляется код баланса оператора сети. Пример: <b>«*100#»</b> - МегаФон.
</ul>
  &nbsp;<b>Режим работы:</b>
<p>&nbsp;
  Выбирается отправкой смс с номера телефона админа или дублера на прибор с соответствующим номером режима работы. Пример: смс с текстом <b>«1»</b> активирует режим отслеживания, при дальнейших переключениях режимов прибора автоматически загружается режим выбранный по смс, после чего этот режим прописывается в память до следующей смены с помощью смс. Для смены режимов с помощью смс не обязательно предварительно отключать режим отслеживания, просто отправляется смс с номером желаемого режима работы и устройство оповестит вас о смене режима работы. Для для отключения отслеживания с помощью смс достаточно отправить на номер устройства смс с текстом <b>«0»</b>. Так же включение и отключение отслеживания производится пустым звонком на устройство.
</p>
&nbsp;<b>Светозвуковая сигнализация:</b>
<ul>
  <li>повторяющиеся серии коротких световых сигналов - входящий звонок
  <li>инициализация датчиков - свечение 3 секунды для каждого датчика с интервалом 100 миллисекунд
  <li>отправка уведомления - свечение 2 секунды
  <li>после включения или перезапуска устройства и инициализации датчиков производится отправка смс уведомления о перезагрузке, светодиод горит 1 секунду, гаснет, и если всё в порядке, то это последний световой сигнал указывающий на успешный переход в рабочий режим
</ul>

